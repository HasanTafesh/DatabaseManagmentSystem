#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "../include/department.h"
#include "../include/instructor.h"
#include "../include/course.h"
#include "../include/student.h"
#include "../include/common.h"
#include "../include/lock_management.h"
#include <unistd.h>  // Required for sleep function


// Global variables
Department **departmentHashTable = NULL; // Dynamic hash table pointer
DepartmentNameIdMapping departmentNameMapping[NAME_MAPPING_SIZE];
int departmentMappingCount = 0;
int departmentCounter = 0;
int *departmentIdArray;

// Comparison functions for sorting
int compareDepartmentName(const void *a, const void *b) {
    return strcmp(((DepartmentNameIdMapping *)a)->name, ((DepartmentNameIdMapping *)b)->name);
}

int compareDepartmentId(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// Initialize departments
void initDepartments() {
    departmentHashTable = calloc(hashTableSize, sizeof(Department *));
    if (!departmentHashTable) {
        printf("Memory allocation failed for department hash table.\n");
        exit(EXIT_FAILURE);
    }

    departmentIdArray = malloc(hashTableSize * sizeof(int));
    if (!departmentIdArray) {
        printf("Memory allocation failed for department ID array.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < hashTableSize; i++) {
        departmentIdArray[i] = -1;
    }

    FILE *file = fopen("data/Departments.txt", "r");
    if (file) {
        int id;
        char name[100], phone[15];

        while (fscanf(file, "%d %s %s\n", &id, name, phone) != EOF) {
            Department *dept = malloc(sizeof(Department));
            if (!dept) {
                printf("Memory allocation failed for department entry.\n");
                fclose(file);
                return;
            }
            dept->id = id;
            strncpy(dept->name, name, 100);
            strncpy(dept->phone, phone, 15);
            dept->occupied = 1;

            insertDepartment(dept, true);
        }
        fclose(file);
    }
}

// Validation functions
bool validateDepartmentData(Department *dept) {
    if (strlen(dept->name) == 0 || strlen(dept->name) > 99) {
        printf("Error: Department name must be between 1 and 99 characters.\n");
        return false;
    }
    if (strlen(dept->phone) == 0 || strlen(dept->phone) > 14) {
        printf("Error: Phone number must be between 1 and 14 characters.\n");
        return false;
    }
    for (int i = 0; i < strlen(dept->phone); i++) {
        if (i == 0 && dept->phone[i] == '+') continue;
        if (!isdigit(dept->phone[i])) {
            printf("Error: Phone number can only contain digits and optional + prefix.\n");
            return false;
        }
    }
    return true;
}

// Insert new department
void insertDepartment(Department *dept, bool isInit) {
    acquire_lock(3, EXCLUSIVE); // Lock for exclusive access to department data

    if (!isInit && !validateDepartmentData(dept)) {
        release_lock(3, EXCLUSIVE);
        return;
    }

    // Check for duplicate ID
    if (bsearch(&(dept->id), departmentIdArray, departmentCounter, sizeof(int), compareDepartmentId) != NULL) {
        printf("\nFailed to add department. A department with ID %d already exists.\n", dept->id);
        release_lock(3, EXCLUSIVE);
        return;
    }

    // Resize hash table if load factor exceeds threshold
    if ((float)departmentCounter / hashTableSize > 0.75) { 
        int newSize = hashTableSize * 2; 
        Department **newHashTable = calloc(newSize, sizeof(Department *));
        if (!newHashTable) {
            printf("Error: Memory allocation failed during hash table resizing.\n");
            release_lock(3, EXCLUSIVE);
            return;
        }

        // Rehash existing entries into the new table
        for (int i = 0; i < hashTableSize; i++) {
            if (departmentHashTable[i] != NULL && departmentHashTable[i]->occupied) {
                int newIndex = departmentHashTable[i]->id % newSize;
                while (newHashTable[newIndex] != NULL) {
                    newIndex = (newIndex + 1) % newSize;
                }
                newHashTable[newIndex] = departmentHashTable[i];
            }
        }

        free(departmentHashTable);
        departmentHashTable = newHashTable;
        hashTableSize = newSize;
    }
    
    // Resize ID array if necessary
    if (departmentCounter == hashTableSize) {
        int *newIdArray = realloc(departmentIdArray, hashTableSize * 2 * sizeof(int));
        if (!newIdArray) {
            printf("Error: Memory allocation failed for department ID array resizing.\n");
            release_lock(3, EXCLUSIVE);
            return;
        }
        departmentIdArray = newIdArray;
    }

    // Add ID to array and sort
    departmentIdArray[departmentCounter++] = dept->id;
    qsort(departmentIdArray, departmentCounter, sizeof(int), compareDepartmentId);

    // Insert department into hash table
    int index = hash(dept->id) % hashTableSize;
    int originalIndex = index;
    while (departmentHashTable[index] != NULL) {
        if (!departmentHashTable[index]->occupied) { // Reuse unoccupied slot
            free(departmentHashTable[index]);
            break;
        }
        index = (index + 1) % hashTableSize;
        if (index == originalIndex) { 
            printf("Error: Hash table is full. Cannot insert department.\n");
            release_lock(3, EXCLUSIVE);
            return;
        }
    }
    departmentHashTable[index] = dept;
    departmentHashTable[index]->occupied = 1;

    // Add to name mapping array
    if (departmentMappingCount < NAME_MAPPING_SIZE) {
        strncpy(departmentNameMapping[departmentMappingCount].name, dept->name, 50);
        departmentNameMapping[departmentMappingCount].id = dept->id;
        departmentMappingCount++;
        qsort(departmentNameMapping, departmentMappingCount, sizeof(DepartmentNameIdMapping), compareDepartmentName);
    }

    // Store department in file if not during initialization
    if (!isInit) {
        FILE *file = fopen("data/Departments.txt", "a");
        if (file) {
            fprintf(file, "%d %s %s\n", dept->id, dept->name, dept->phone);
            fflush(file);
            fclose(file);
            printf("\nDepartment added successfully!\n");
        } else {
            perror("Failed to write to Departments.txt");
        }
    }

    release_lock(3, EXCLUSIVE); // Release lock after insertion
}




// Update department
void updateDepartment(int id, char *phone) {
    acquire_lock(3, EXCLUSIVE); // Lock for updating a department

    Department *dept = searchDepartmentById(id);
    if (dept != NULL) {
        if (strlen(phone) == 0 || strlen(phone) > 14) {
            printf("Error: Invalid phone number length\n");
            release_lock(3, EXCLUSIVE); // Unlock if validation fails
            return;
        }

        for (int i = 0; i < strlen(phone); i++) {
            if (i == 0 && phone[i] == '+') continue;
            if (!isdigit(phone[i])) {
                printf("Error: Invalid phone number format\n");
                release_lock(3, EXCLUSIVE); // Unlock if validation fails
                return;
            }
        }

        strncpy(dept->phone, phone, 15);
        printf("\nPhone number updated successfully.\n");

        // Update file
        FILE *file = fopen("data/Departments.txt", "r");
        FILE *temp = fopen("data/Departments_temp.txt", "w");

        if (file != NULL && temp != NULL) {
            int curr_id;
            char name[100], curr_phone[15];

            while (fscanf(file, "%d %s %s\n", &curr_id, name, curr_phone) != EOF) {
                if (curr_id == id) {
                    fprintf(temp, "%d %s %s\n", id, name, phone);
                } else {
                    fprintf(temp, "%d %s %s\n", curr_id, name, curr_phone);
                }
            }

            fclose(file);
            fclose(temp);
            remove("data/Departments.txt");
            rename("data/Departments_temp.txt", "data/Departments.txt");
        }
    } else {
        printf("Department not found.\n");
    }

    release_lock(3, EXCLUSIVE); // Unlock after updating
}

// Delete department
void deleteDepartment(int id) {
    acquire_lock(3, EXCLUSIVE); // Lock for deleting a department

    int index = hash(id);
    while (departmentHashTable[index] != NULL) {
        if (departmentHashTable[index]->occupied && departmentHashTable[index]->id == id) {
            // Check for referential integrity
            for (int i = 0; i < HASH_TABLE_SIZE; i++) {
                if (instructorHashTable[i] != NULL && instructorHashTable[i]->occupied &&
                    instructorHashTable[i]->departmentId == id) {
                    printf("Error: Cannot delete department - Instructors are assigned\n");
                    release_lock(3, EXCLUSIVE); // Unlock if constraints not met
                    return;
                }
                if (courseHashTable[i] != NULL && courseHashTable[i]->occupied &&
                    courseHashTable[i]->departmentId == id) {
                    printf("Error: Cannot delete department - Courses are assigned\n");
                    release_lock(3, EXCLUSIVE); // Unlock if constraints not met
                    return;
                }
                if (studentHashTable[i] != NULL && studentHashTable[i]->occupied &&
                    studentHashTable[i]->departmentId == id) {
                    printf("Error: Cannot delete department - Students are enrolled\n");
                    release_lock(3, EXCLUSIVE); // Unlock if constraints not met
                    return;
                }
            }

            departmentHashTable[index]->occupied = 0;
            printf("\nDepartment %s deleted successfully!\n", departmentHashTable[index]->name);

            // Remove from mapping array
            for (int i = 0; i < departmentMappingCount; i++) {
                if (departmentNameMapping[i].id == id) {
                    for (int j = i; j < departmentMappingCount - 1; j++) {
                        departmentNameMapping[j] = departmentNameMapping[j + 1];
                    }
                    departmentMappingCount--;
                    break;
                }
            }

            // Remove from ID array
            int *found = bsearch(&id, departmentIdArray, departmentCounter, sizeof(int), compareDepartmentId);
            if (found != NULL) {
                memmove(found, found + 1, (--departmentCounter - (found - departmentIdArray)) * sizeof(int));
            }

            // Update file
            FILE *file = fopen("data/Departments.txt", "r");
            FILE *temp = fopen("data/Departments_temp.txt", "w");

            if (file != NULL && temp != NULL) {
                int curr_id;
                char name[100], phone[15];

                while (fscanf(file, "%d %s %s\n", &curr_id, name, phone) != EOF) {
                    if (curr_id != id) {
                        fprintf(temp, "%d %s %s\n", curr_id, name, phone);
                    }
                }

                fclose(file);
                fclose(temp);
                remove("data/Departments.txt");
                rename("data/Departments_temp.txt", "data/Departments.txt");
            }

            release_lock(3, EXCLUSIVE); // Unlock after deletion
            return;
        }
        index = hash(index + 1);
    }
    printf("Department not found.\n");
    release_lock(3, EXCLUSIVE);
}

// Search functions
Department *searchDepartmentById(int id) {

    int index = hash(id);
    while (departmentHashTable[index] != NULL) {
        if (departmentHashTable[index]->occupied && departmentHashTable[index]->id == id) {
            return departmentHashTable[index];
        }
        index = hash(index + 1);
    }

    return NULL;
}

Department *searchDepartmentByName(char name[]) {

    for (int i = 0; i < departmentMappingCount; i++) {
        if (strcmp(departmentNameMapping[i].name, name) == 0) {
            return searchDepartmentById(departmentNameMapping[i].id);
        }
    }

    return NULL;
}

Department *searchDepartmentByPhone(char phone[]) {

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (departmentHashTable[i] != NULL && departmentHashTable[i]->occupied &&
            strcmp(departmentHashTable[i]->phone, phone) == 0) {
            return departmentHashTable[i];
        }
    }

    return NULL;
}

// Display functions
void *printDepartment(void *arg) {
    acquire_lock(3, SHARED); // Lock for reading

    DepartmentThreadArg *threadArg = (DepartmentThreadArg *)arg;
    Department *dept = threadArg->department;

    // printf("Thread %lu is processing:\n", pthread_self());
    printf("ID: %d Name: %s Phone: %s\n",
           dept->id, dept->name, dept->phone);

    // Simulate processing time
    sleep(1);

    // printf("Thread %lu finished processing department: %s\n", pthread_self(), dept->name);

    release_lock(3, SHARED);
    return NULL;
}

// Function to display all departments
void showAllDepartments() {

        //  initDepartments();

    acquire_lock(3, SHARED); // Lock for reading


    printf("\nThere are currently %d department(s) in the database.\n", departmentCounter);

    pthread_t threads[departmentCounter];
    DepartmentThreadArg args[departmentCounter];
    int threadIndex = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (departmentHashTable[i] != NULL && departmentHashTable[i]->occupied) {
            args[threadIndex].department = departmentHashTable[i];
            pthread_create(&threads[threadIndex], NULL, printDepartment, &args[threadIndex]);
            threadIndex++;
        }
    }

    for (int i = 0; i < threadIndex; i++) {
        pthread_join(threads[i], NULL);
    }

    release_lock(3, SHARED);
}

void showDepartment(Department *dept) {
    if (!dept) {
        printf("No department data to display.\n");
        return;
    }

    printf("\n*********************************************\n");
    printf("ID: %d\n", dept->id);
    printf("Name: %s\n", dept->name);
    printf("Phone: %s\n", dept->phone);
}

// Menu operations
Department *selectDepartmentMenu() {
    int choice;
    do {
        printf("\nSelect Department:\n");
        printf("1. Search by ID\n");
        printf("2. Search by Name\n");
        printf("3. Search by Phone\n");
        printf("0. Cancel\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        int id;
        char name[100], phone[15];
        Department *dept = NULL;
        
        switch(choice) {
            case 1:
                printf("Enter Department ID: ");
                scanf("%d", &id);
                getchar();
                dept = searchDepartmentById(id);
                break;
            case 2:
                printf("Enter Department Name: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                dept = searchDepartmentByName(name);
                break;
            case 3:
                printf("Enter Phone Number: ");
                fgets(phone, sizeof(phone), stdin);
                phone[strcspn(phone, "\n")] = 0;
                dept = searchDepartmentByPhone(phone);
                break;
            case 0:
                return NULL;
            default:
                printf("Invalid choice.\n");
                return NULL;
        }
        
        if (dept) {
            showDepartment(dept);
            return dept;
        } else {
            printf("Department not found.\n");
        }
    } while (choice != 0);
    
    return NULL;
}

void insertDepartmentMenu() {
    Department *dept = malloc(sizeof(Department));
    
    printf("\nAdd New Department\n");
    printf("Enter Department ID: ");
    scanf("%d", &dept->id);
    getchar();
    
    printf("Enter Department Name: ");
    fgets(dept->name, sizeof(dept->name), stdin);
    dept->name[strcspn(dept->name, "\n")] = 0;
    
    printf("Enter Phone Number: ");
    fgets(dept->phone, sizeof(dept->phone), stdin);
    dept->phone[strcspn(dept->phone, "\n")] = 0;
    
    insertDepartment(dept, false);
}

void updateDepartmentMenu() {
    Department *dept = selectDepartmentMenu();
    if (dept == NULL) return;

    char phone[15];
    printf("Enter new phone number: ");
    fgets(phone, sizeof(phone), stdin);
    phone[strcspn(phone, "\n")] = 0;
    
    updateDepartment(dept->id, phone);
}

void deleteDepartmentMenu() {
    Department *dept = selectDepartmentMenu();
    if (dept != NULL) {
        deleteDepartment(dept->id);
    }
}

void departmentMenu() {
    int choice;
    do {
        printf("\nDepartment Operations\n");
        printf("1. Show All Departments\n");
        printf("2. Show Department\n");
        printf("3. Add Department\n");
        printf("4. Update Department\n");
        printf("5. Delete Department\n");
        printf("6. Show Department Instructors\n");
        printf("7. Show Department Courses\n");
        printf("8. Show Department Students\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        switch(choice) {
            case 1:
                showAllDepartments();
                break;
            case 2:
                selectDepartmentMenu();
                break;
            case 3:
                insertDepartmentMenu();
                break;
            case 4:
                updateDepartmentMenu();
                break;
            case 5:
                deleteDepartmentMenu();
                break;
            case 6:
                {
                    Department *dept = selectDepartmentMenu();
                    if (dept) showInstructorsInDepartment(dept->id);
                }
                break;
            case 7:
                {
                    Department *dept = selectDepartmentMenu();
                    if (dept) showCoursesInDepartment(dept->id);
                }
                break;
            case 8:
                {
                    Department *dept = selectDepartmentMenu();
                    if (dept) showStudentsInDepartment(dept->id);
                }
                break;
            case 0:
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

// Additional functions
void showInstructorsInDepartment(int departmentId) {
    acquire_lock(3, SHARED); // Lock for reading

    printf("\nInstructors in Department %d:\n", departmentId);
    bool found = false;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (instructorHashTable[i] != NULL && instructorHashTable[i]->occupied &&
            instructorHashTable[i]->departmentId == departmentId) {
            showInstructor(instructorHashTable[i]);
            found = true;
        }
    }

    if (!found) {
        printf("No instructors found in this department.\n");
    }

    release_lock(3, SHARED);
}

void showCoursesInDepartment(int departmentId) {
    acquire_lock(3, SHARED); // Lock for reading

    printf("\nCourses in Department %d:\n", departmentId);
    bool found = false;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (courseHashTable[i] != NULL && courseHashTable[i]->occupied &&
            courseHashTable[i]->departmentId == departmentId) {
            showCourse(courseHashTable[i]);
            found = true;
        }
    }

    if (!found) {
        printf("No courses found in this department.\n");
    }

    release_lock(3, SHARED);
}

void showStudentsInDepartment(int departmentId) {
    acquire_lock(3, SHARED); // Lock for reading

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

    release_lock(3, SHARED);
}
