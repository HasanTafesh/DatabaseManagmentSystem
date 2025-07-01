#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "../include/student.h"
#include "../include/department.h"
#include "../include/course.h"
#include "../include/enrollment.h"
#include "../include/common.h"
#include "../include/lock_management.h"

// Global variables
Student **studentHashTable = NULL; // Dynamic hash table pointer
StudentNameIdMapping studentNameMapping[NAME_MAPPING_SIZE];
int studentMappingCount = 0;
int studentCounter = 0;
int *studentIdArray;

// Comparison functions for sorting
int compareStudentName(const void *a, const void *b) {
    const StudentNameIdMapping *m1 = (const StudentNameIdMapping *)a;
    const StudentNameIdMapping *m2 = (const StudentNameIdMapping *)b;
    int result = strcmp(m1->firstName, m2->firstName);
    if (result == 0) {
        return strcmp(m1->lastName, m2->lastName);
    }
    return result;
}

int compareStudentId(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// Initialize students
void initStudents() {
    // Initialize hash table
    studentHashTable = calloc(hashTableSize, sizeof(Student *));
    if (studentHashTable == NULL) {
        printf("Memory allocation failed for student hash table.\n");
        exit(EXIT_FAILURE);
    }

    // Initialize ID array
    studentIdArray = malloc(hashTableSize * sizeof(int));
    if (studentIdArray == NULL) {
        printf("Memory allocation failed for student ID array.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < hashTableSize; i++) {
        studentIdArray[i] = -1;
    }

    // Read from file
    FILE *file = fopen("data/Students.txt", "r");
    if (file != NULL) {
        int id;
        char firstName[50];
        char lastName[50];
        char email[100];
        char phone[15];
        int departmentId;

        while (fscanf(file, "%d %s %s %s %s %d\n", &id, firstName, lastName, 
                     email, phone, &departmentId) != EOF) {
            Student *student = malloc(sizeof(Student));
            student->id = id;
            strncpy(student->firstName, firstName, 50);
            strncpy(student->lastName, lastName, 50);
            strncpy(student->email, email, 100);
            strncpy(student->phone, phone, 15);
            student->departmentId = departmentId;
            student->occupied = 1;

            insertStudent(student, true);
        }
        fclose(file);
    }
}


// Validation functions
bool validateStudentData(Student *student) {
    if (strlen(student->firstName) == 0 || strlen(student->firstName) > 49) {
        printf("Error: First name must be between 1 and 49 characters.\n");
        return false;
    }
    if (strlen(student->lastName) == 0 || strlen(student->lastName) > 49) {
        printf("Error: Last name must be between 1 and 49 characters.\n");
        return false;
    }
    if (strlen(student->email) == 0 || strlen(student->email) > 99) {
        printf("Error: Email must be between 1 and 99 characters.\n");
        return false;
    }
    if (!strchr(student->email, '@') || !strchr(student->email, '.')) {
        printf("Error: Invalid email format.\n");
        return false;
    }
    if (strlen(student->phone) == 0 || strlen(student->phone) > 14) {
        printf("Error: Phone number must be between 1 and 14 characters.\n");
        return false;
    }
    for (int i = 0; i < strlen(student->phone); i++) {
        if (i == 0 && student->phone[i] == '+') continue;
        if (!isdigit(student->phone[i])) {
            printf("Error: Phone number can only contain digits and optional + prefix.\n");
            return false;
        }
    }
    return true;
}

// Insert new student
void insertStudent(Student *student, bool isInit) {
    acquire_lock(1, EXCLUSIVE); // Lock for exclusive access to the student hash table

    // Validate student data if not during initialization
    if (!isInit && !validateStudentData(student)) {
        release_lock(1, EXCLUSIVE);
        return;
    }

    // Check for duplicate ID
    if (bsearch(&(student->id), studentIdArray, studentCounter, sizeof(int), compareStudentId) != NULL) {
        printf("\nFailed to add student. A student with ID %d already exists.\n", student->id);
        release_lock(1, EXCLUSIVE);
        return;
    }

    // Check department reference
    if (searchDepartmentById(student->departmentId) == NULL) {
        printf("\nFailed to add student. Department %d does not exist.\n", student->departmentId);
        release_lock(1, EXCLUSIVE);
        return;
    }

    // Resize hash table if load factor exceeds threshold
    if ((float)studentCounter / hashTableSize > 0.75) { // Load factor > 0.75
        int newSize = hashTableSize * 2; // Double the size
        Student **newHashTable = calloc(newSize, sizeof(Student *));
        if (newHashTable == NULL) {
            printf("Error: Memory allocation failed during hash table resizing.\n");
            release_lock(1, EXCLUSIVE);
            return;
        }

        // Rehash existing entries into the new table
        for (int i = 0; i < hashTableSize; i++) {
            if (studentHashTable[i] != NULL && studentHashTable[i]->occupied == 1) {
                int newIndex = studentHashTable[i]->id % newSize;
                while (newHashTable[newIndex] != NULL) {
                    newIndex = (newIndex + 1) % newSize;
                }
                newHashTable[newIndex] = studentHashTable[i];
            }
        }

        // Free the old table and update the pointer
        free(studentHashTable);
        studentHashTable = newHashTable;
        hashTableSize = newSize; // Update the hash table size
    }

    // Resize ID array if needed
    if (studentCounter == hashTableSize) {
        int *newIdArray = realloc(studentIdArray, hashTableSize * 2 * sizeof(int));
        if (newIdArray == NULL) {
            printf("Error: Memory allocation failed for student ID array resizing.\n");
            release_lock(1, EXCLUSIVE);
            return;
        }
        studentIdArray = newIdArray;
    }

    // Add ID to array and sort
    studentIdArray[studentCounter++] = student->id;
    qsort(studentIdArray, studentCounter, sizeof(int), compareStudentId);

    // Insert student into hash table
    int index = hash(student->id) % hashTableSize;
    int originalIndex = index;
    while (studentHashTable[index] != NULL) {
        if (studentHashTable[index]->occupied == 0) { // Reuse a deleted slot
            free(studentHashTable[index]);
            break;
        }
        index = (index + 1) % hashTableSize; // Handle collisions with linear probing
        if (index == originalIndex) { // Full loop
            printf("Error: Hash table is full. Cannot insert student.\n");
            release_lock(1, EXCLUSIVE);
            return;
        }
    }
    studentHashTable[index] = student;
    studentHashTable[index]->occupied = 1;

    // Add to name mapping
    if (studentMappingCount < NAME_MAPPING_SIZE) {
        strncpy(studentNameMapping[studentMappingCount].firstName, student->firstName, 50);
        strncpy(studentNameMapping[studentMappingCount].lastName, student->lastName, 50);
        studentNameMapping[studentMappingCount].id = student->id;
        studentMappingCount++;
        qsort(studentNameMapping, studentMappingCount, sizeof(StudentNameIdMapping), compareStudentName);
    }

    // Store student to file if not during initialization
    if (!isInit) {
        FILE *file = fopen("data/Students.txt", "a");
        if (file != NULL) {
            fprintf(file, "%d %s %s %s %s %d\n",
                    student->id, student->firstName, student->lastName,
                    student->email, student->phone, student->departmentId);
            fflush(file);
            fclose(file);
            printf("\nStudent added successfully!\n");
        } else {
            perror("Error opening file Students.txt for appending");
        }
    }

    release_lock(1, EXCLUSIVE); // Release the lock
}


// Update student
void updateStudent(int id, char *phone) {

    acquire_lock(1, EXCLUSIVE);

    Student *student = searchStudentById(id);
    if (student != NULL) {
        strncpy(student->phone, phone, 15);
        printf("\nPhone number updated successfully.\n");
        
        // Update file
        FILE *file = fopen("data/Students.txt", "r");
        FILE *temp = fopen("data/Students_temp.txt", "w");
        
        if (file != NULL && temp != NULL) {
            int curr_id;
            char firstName[50], lastName[50], email[100], curr_phone[15];
            int departmentId;

            while (fscanf(file, "%d %s %s %s %s %d\n", &curr_id, firstName, lastName, 
                         email, curr_phone, &departmentId) != EOF) {
                if (curr_id == id) {
                    fprintf(temp, "%d %s %s %s %s %d\n", id, firstName, lastName,
                            email, phone, departmentId);
                } else {
                    fprintf(temp, "%d %s %s %s %s %d\n", curr_id, firstName, lastName,
                            email, curr_phone, departmentId);
                }
            }

            fclose(file);
            fclose(temp);
            remove("data/Students.txt");
            rename("data/Students_temp.txt", "data/Students.txt");
        }
    } else {
        printf("Student not found.\n");
    }

    release_lock(1, EXCLUSIVE);
}

// Delete student
void deleteStudent(int id) {

    acquire_lock(1, EXCLUSIVE);

    int index = hash(id);
    while (studentHashTable[index] != NULL) {
        if (studentHashTable[index]->occupied && studentHashTable[index]->id == id) {
            studentHashTable[index]->occupied = 0;
            printf("\nStudent %s deleted successfully!\n", studentHashTable[index]->firstName);

            // Remove from mapping array
            for (int i = 0; i < studentMappingCount; i++) {
                if (studentNameMapping[i].id == id) {
                    for (int j = i; j < studentMappingCount - 1; j++) {
                        studentNameMapping[j] = studentNameMapping[j + 1];
                    }
                    studentMappingCount--;
                    break;
                }
            }

            // Remove from ID array
            int *found = bsearch(&id, studentIdArray, studentCounter, sizeof(int), compareStudentId);
            if (found != NULL) {
                memmove(found, found + 1, (--studentCounter - (found - studentIdArray)) * sizeof(int));
            }

            // Update file
            FILE *file = fopen("data/Students.txt", "r");
            FILE *temp = fopen("data/Students_temp.txt", "w");
            
            if (file != NULL && temp != NULL) {
                int curr_id;
                char firstName[50], lastName[50], email[100], phone[15];
                int departmentId;

                while (fscanf(file, "%d %s %s %s %s %d\n", &curr_id, firstName, lastName, 
                             email, phone, &departmentId) != EOF) {
                    if (curr_id != id) {
                        fprintf(temp, "%d %s %s %s %s %d\n", curr_id, firstName, lastName,
                                email, phone, departmentId);
                    }
                }

                fclose(file);
                fclose(temp);
                remove("data/Students.txt");
                rename("data/Students_temp.txt", "data/Students.txt");


                release_lock(1, EXCLUSIVE);
            }

            return;
        }
        index = hash(index + 1);
    }
    printf("Student not found.\n");
    release_lock(1, EXCLUSIVE);
}

// Search functions
Student *searchStudentById(int id) {
    int index = hash(id);
    while (studentHashTable[index] != NULL) {
        if (studentHashTable[index]->occupied && studentHashTable[index]->id == id) {
            return studentHashTable[index];
        }
        index = hash(index + 1);
    }
    return NULL;
}

Student *searchStudentByName(char firstName[], char lastName[]) {
    for (int i = 0; i < studentMappingCount; i++) {
        if (strcmp(studentNameMapping[i].firstName, firstName) == 0 &&
            strcmp(studentNameMapping[i].lastName, lastName) == 0) {
            return searchStudentById(studentNameMapping[i].id);
        }
    }
    return NULL;
}

Student *searchStudentByEmail(char email[]) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (studentHashTable[i] != NULL && studentHashTable[i]->occupied &&
            strcmp(studentHashTable[i]->email, email) == 0) {
            return studentHashTable[i];
        }
    }
    return NULL;
}

Student *searchStudentByPhone(char phone[]) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (studentHashTable[i] != NULL && studentHashTable[i]->occupied &&
            strcmp(studentHashTable[i]->phone, phone) == 0) {
            return studentHashTable[i];
        }
    }
    return NULL;
}

void searchStudentsByDepartment(int departmentId) {
    printf("\nStudents in Department %d:\n", departmentId);
    bool found = false;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (studentHashTable[i] != NULL && studentHashTable[i]->occupied &&
            studentHashTable[i]->departmentId == departmentId) {
            showStudent(studentHashTable[i]);
            found = true;
        }
    }


    if (!found) {
        printf("No students found in this department.\n");
    }
}

// Display functions
void *printStudent(void *arg) {
    StudentThreadArg *threadArg = (StudentThreadArg *)arg;
    Student *student = threadArg->student;


    acquire_lock(1, SHARED);
    printf("ID: %d Name: %s %s Email: %s Phone: %s Department ID: %d\n",
           student->id, student->firstName, student->lastName,
           student->email, student->phone, student->departmentId);

    release_lock(1, SHARED);
    return NULL;
}

void showAllStudents() {

    acquire_lock(1, SHARED);
    printf("\nThere are currently %d student(s) in the database.\n", studentCounter);

    pthread_t threads[studentCounter];
    StudentThreadArg args[studentCounter];
    int threadIndex = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (studentHashTable[i] != NULL && studentHashTable[i]->occupied) {
            args[threadIndex].student = studentHashTable[i];
            pthread_create(&threads[threadIndex], NULL, printStudent, &args[threadIndex]);
            threadIndex++;
        }
    }

    for (int i = 0; i < threadIndex; i++) {
        pthread_join(threads[i], NULL);
    }
    release_lock(1, SHARED);
}

void showStudent(Student *student) {
    if (!student) {
        printf("No student data to display.\n");
        return;
    }


    printf("\n*********************************************\n");
    printf("ID: %d\n", student->id);
    printf("Name: %s %s\n", student->firstName, student->lastName);
    printf("Email: %s\n", student->email);
    printf("Phone: %s\n", student->phone);
    printf("Department ID: %d\n", student->departmentId);

}

// Menu operations
Student *selectStudentMenu() {
    int choice;
    do {
        printf("\nSelect Student:\n");
        printf("1. Search by ID\n");
        printf("2. Search by Name\n");
        printf("3. Search by Email\n");
        printf("4. Search by Phone\n");
        printf("0. Cancel\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        int id;
        char firstName[50], lastName[50], email[100], phone[15];
        Student *student = NULL;
        
        switch(choice) {
            case 1:
                acquire_lock(1, SHARED);
                printf("Enter Student ID: ");
                scanf("%d", &id);
                getchar();
                student = searchStudentById(id);
                release_lock(1, SHARED);
                break;
            case 2:
                acquire_lock(1, SHARED);
                printf("Enter First Name: ");
                fgets(firstName, sizeof(firstName), stdin);
                firstName[strcspn(firstName, "\n")] = 0;
                
                printf("Enter Last Name: ");
                fgets(lastName, sizeof(lastName), stdin);
                lastName[strcspn(lastName, "\n")] = 0;
                
                student = searchStudentByName(firstName, lastName);
                release_lock(1, SHARED);
                break;
            case 3:
                acquire_lock(1, SHARED);
                printf("Enter Email: ");
                fgets(email, sizeof(email), stdin);
                email[strcspn(email, "\n")] = 0;
                student = searchStudentByEmail(email);
                release_lock(1, SHARED);
                break;
            case 4:
                acquire_lock(1, SHARED);
                printf("Enter Phone: ");
                fgets(phone, sizeof(phone), stdin);
                phone[strcspn(phone, "\n")] = 0;
                student = searchStudentByPhone(phone);
                release_lock(1, SHARED);
                break;
            case 0:
                return NULL;
            default:
                printf("Invalid choice.\n");
                return NULL;
        }
        
        if (student) {
            acquire_lock(1, SHARED);
            showStudent(student);
            release_lock(1, SHARED);
            return student;
        } else {
            printf("Student not found.\n");
        }
    } while (choice != 0);
    
    return NULL;
}


void insertStudentMenu() {
    Student *student = malloc(sizeof(Student));
    
    printf("\nAdd New Student\n");
    printf("Enter Student ID: ");
    scanf("%d", &student->id);
    getchar();
    
    printf("Enter First Name: ");
    fgets(student->firstName, sizeof(student->firstName), stdin);
    student->firstName[strcspn(student->firstName, "\n")] = 0;
    
    printf("Enter Last Name: ");
    fgets(student->lastName, sizeof(student->lastName), stdin);
    student->lastName[strcspn(student->lastName, "\n")] = 0;
    
    printf("Enter Email: ");
    fgets(student->email, sizeof(student->email), stdin);
    student->email[strcspn(student->email, "\n")] = 0;
    
    printf("Enter Phone: ");
    fgets(student->phone, sizeof(student->phone), stdin);
    student->phone[strcspn(student->phone, "\n")] = 0;
    
    printf("Enter Department ID: ");
    scanf("%d", &student->departmentId);
    getchar();
    
    insertStudent(student, false);
}

void updateStudentMenu() {
    Student *student = selectStudentMenu();
    if (student == NULL) return;

    char phone[15];
    printf("Enter new phone number: ");
    fgets(phone, sizeof(phone), stdin);
    phone[strcspn(phone, "\n")] = 0;
    
    updateStudent(student->id, phone);
}

void deleteStudentMenu() {
    Student *student = selectStudentMenu();
    if (student != NULL) {
        deleteStudent(student->id);
    }
}

void studentMenu() {
    int choice;
    do {
        printf("\nStudent Operations\n");
        printf("1. Show All Students\n");
        printf("2. Show Student\n");
        printf("3. Add Student\n");
        printf("4. Update Student\n");
        printf("5. Delete Student\n");
        printf("6. View Student Courses\n");
        printf("7. View Grades\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        switch(choice) {
            case 1:
                showAllStudents();
                break;
            case 2:
                selectStudentMenu();
                break;
            case 3:
                insertStudentMenu();
                break;
            case 4:
                updateStudentMenu();
                break;
            case 5:
                deleteStudentMenu();
                break;
            case 6:
                {
                    Student *student = selectStudentMenu();
                    if (student) showStudentCourses(student->id);
                }
                break;
            case 7:
                {
                    Student *student = selectStudentMenu();
                    if (student) showStudentGrades(student->id);
                }
                break;
            case 0:
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (choice != 0);
}



void showStudentCourses(int studentId) {
    acquire_lock(2, SHARED);

    printf("\nCourses for Student %d:\n", studentId);

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {

        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied && enrollmentHashTable[i]->studentId == studentId) {

            Course *course = searchCourseById(enrollmentHashTable[i]->courseId);
            if (course != NULL) {
                printf("Course ID: %d\n", course->id);
                printf("Title: %s\n", course->title);
                printf("Credits: %d\n", course->credits);
                printf("Status: %s\n", getStatusString(enrollmentHashTable[i]->status));
                printf("-------------------------\n");
            }
        }
    }
    release_lock(2, SHARED);
}

void showStudentGrades(int studentId) {

    acquire_lock(2, SHARED);
    printf("\nGrades for Student %d:\n", studentId);

    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->studentId == studentId) {
            Course *course = searchCourseById(enrollmentHashTable[i]->courseId);
            if (course != NULL) {
                printf("Course: %s (ID: %d)\n", course->title, course->id);
                printf("Credits: %d\n", course->credits);
                printf("Grade: %s\n", enrollmentHashTable[i]->grade);
                printf("-------------------------\n");
            }
        }
    }

    release_lock(2, SHARED);

}
