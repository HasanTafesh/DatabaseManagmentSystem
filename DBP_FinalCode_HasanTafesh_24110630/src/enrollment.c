#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include "../include/enrollment.h"
#include "../include/student.h"
#include "../include/course.h"
#include "../include/common.h"
#include "../include/lock_management.h"

// Global variables
Enrollment **enrollmentHashTable = NULL; // Dynamic hash table pointer
int *enrollmentIdArray = NULL; // Dynamic array for enrollment IDs
int enrollmentCounter = 0;

// Comparison function for sorting enrollment IDs
int compareEnrollmentId(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// Initializing the enrollments
void initEnrollments() {
    // Allocating memory dynamically for the hash table
    enrollmentHashTable = calloc(hashTableSize, sizeof(Enrollment *));
    if (enrollmentHashTable == NULL) {
        printf("Memory allocation failed for enrollment hash table.\n");
        exit(EXIT_FAILURE);
    }

    // Allocating memory dynamically for the ID array
    enrollmentIdArray = malloc(hashTableSize * sizeof(int));
    if (enrollmentIdArray == NULL) {
        printf("Memory allocation failed for enrollment ID array.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < hashTableSize; i++) {
        enrollmentIdArray[i] = -1;
    }

    // getting the enrollments from file
    FILE *file = fopen("data/Enrollments.txt", "r");
    if (file != NULL) {
        int id, studentId, courseId, status;
        char grade[MAX_GRADE_LENGTH + 1];

        while (fscanf(file, "%d %d %d %s %d\n", &id, &studentId, &courseId, grade, &status) != EOF) {
            Enrollment *enrollment = malloc(sizeof(Enrollment));
            enrollment->id = id;
            enrollment->studentId = studentId;
            enrollment->courseId = courseId;
            strncpy(enrollment->grade, grade, MAX_GRADE_LENGTH + 1);
            enrollment->status = (EnrollmentStatus)status;
            enrollment->occupied = 1;

            insertEnrollment(enrollment, true);
        }
        fclose(file);
    }
}

// Validation functions
bool validateEnrollmentData(Enrollment *enrollment) {
    if (!validateStudentReference(enrollment->studentId)) {
        printf("Error: Invalid student ID\n");
        return false;
    }
    
    if (!validateCourseReference(enrollment->courseId)) {
        printf("Error: Invalid course ID\n");
        return false;
    }
    
    return true;
}

// Inserting new enrollment
void insertEnrollment(Enrollment *enrollment, bool isInit) {
    acquire_lock(4, EXCLUSIVE); // execlusive lock to the enrollment data

    // Validate enrollment data if not during initialization
    if (!isInit && !validateEnrollmentData(enrollment)) {
        release_lock(4, EXCLUSIVE);
        return;
    }

    // Check for duplicate ID since IDs should be unique
    if (bsearch(&(enrollment->id), enrollmentIdArray, enrollmentCounter, sizeof(int), compareEnrollmentId) != NULL) {
        printf("\nFailed to add enrollment. An enrollment with ID %d already exists.\n", enrollment->id);
        release_lock(4, EXCLUSIVE);
        return;
    }

    // Resizeing the ID array if necessary
    if (enrollmentCounter == hashTableSize) {
        int newSize = hashTableSize * 2;
        int *newIdArray = realloc(enrollmentIdArray, newSize * sizeof(int));
        if (newIdArray == NULL) {
            printf("Error: Memory allocation failed for enrollment ID array resizing.\n");
            release_lock(4, EXCLUSIVE);
            return;
        }
        enrollmentIdArray = newIdArray;
    }

    // Add ID to array and sort
    enrollmentIdArray[enrollmentCounter++] = enrollment->id;
    qsort(enrollmentIdArray, enrollmentCounter, sizeof(int), compareEnrollmentId);

    // Resizing the hash table if necessary
    if ((float)enrollmentCounter / hashTableSize > 0.75) {
        int newSize = hashTableSize * 2;
        Enrollment **newHashTable = calloc(newSize, sizeof(Enrollment *));
        if (newHashTable == NULL) {
            printf("Error: Memory allocation failed during hash table resizing.\n");
            release_lock(4, EXCLUSIVE);
            return;
        }
        // here we have to rehash the elements
        for (int i = 0; i < hashTableSize; i++) {
            // we only need to rehash the occupied elements
            if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied) {
                int newIndex = enrollmentHashTable[i]->id % newSize;
                while (newHashTable[newIndex] != NULL) {
                    newIndex = (newIndex + 1) % newSize;
                }
                newHashTable[newIndex] = enrollmentHashTable[i];
            }
        }
        // Free the old hash table and update the pointer and size
        free(enrollmentHashTable);
        enrollmentHashTable = newHashTable;
        hashTableSize = newSize;
    }

    // Inserting enrollment into hash table
    int index = hash(enrollment->id) % hashTableSize;
    int originalIndex = index;
    while (enrollmentHashTable[index] != NULL) {
        if (enrollmentHashTable[index]->occupied == 0) { // to reuse a deleted slot
            free(enrollmentHashTable[index]);
            break;
        }
        index = (index + 1) % hashTableSize;
        if (index == originalIndex) { 
            printf("Error: Hash table is full. Cannot insert enrollment.\n");
            release_lock(4, EXCLUSIVE);
            return;
        }
    }
    enrollmentHashTable[index] = enrollment;
    enrollment->occupied = 1;

    // Store enrollment to file if not during initialization
    if (!isInit) {
        FILE *file = fopen("data/Enrollments.txt", "a");
        if (file != NULL) {
            fprintf(file, "%d %d %d %s %d\n",
                    enrollment->id, enrollment->studentId, enrollment->courseId,
                    enrollment->grade, enrollment->status);
            fflush(file);
            fclose(file);
            printf("\nEnrollment added successfully!\n");
        } else {
            perror("Failed to write to Enrollments.txt");
        }
    }

    release_lock(4, EXCLUSIVE); // Release the lock
}


// Update grade
void updateGrade(int enrollmentId, char grade[]) {
    acquire_lock(4, EXCLUSIVE); // Locking to update

    Enrollment *enrollment = searchEnrollmentById(enrollmentId);
    if (enrollment != NULL) {
        if (!validateGrade(grade)) {
            printf("Error: Invalid grade format\n");
            release_lock(4, EXCLUSIVE); // we release the lock if validation fails
            return;
        }

        strncpy(enrollment->grade, grade, MAX_GRADE_LENGTH);
        enrollment->grade[MAX_GRADE_LENGTH] = '\0';
        printf("\nGrade updated successfully.\n");

        // Update file
        FILE *file = fopen("data/Enrollments.txt", "r");
        FILE *temp = fopen("data/Enrollments_temp.txt", "w");

        if (file != NULL && temp != NULL) {
            int curr_id, studentId, courseId;
            char curr_grade[MAX_GRADE_LENGTH + 1];
            int status;

            while (fscanf(file, "%d %d %d %s %d\n", &curr_id, &studentId, &courseId, 
                         curr_grade, &status) != EOF) {
                if (curr_id == enrollmentId) {
                    fprintf(temp, "%d %d %d %s %d\n", curr_id, studentId, courseId,
                            grade, status);
                } else {
                    fprintf(temp, "%d %d %d %s %d\n", curr_id, studentId, courseId,
                            curr_grade, status);
                }
            }

            fclose(file);
            fclose(temp);
            remove("data/Enrollments.txt");
            rename("data/Enrollments_temp.txt", "data/Enrollments.txt");
        }
    } else {
        printf("Enrollment not found.\n");
    }

    release_lock(4, EXCLUSIVE); // Unlock after updating grade
}

// Update status
void updateStatus(int enrollmentId, EnrollmentStatus status) {
    acquire_lock(4, EXCLUSIVE); // Lock for updating status

    Enrollment *enrollment = searchEnrollmentById(enrollmentId);
    if (enrollment != NULL) {
        if (!validateStatus(status)) {
            printf("Error: Invalid status\n");
            release_lock(4, EXCLUSIVE); // relese the lock if validation fails
            return;
        }

        enrollment->status = status;
        printf("\nStatus updated successfully.\n");

        // Update file
        FILE *file = fopen("data/Enrollments.txt", "r");
        FILE *temp = fopen("data/Enrollments_temp.txt", "w");

        if (file != NULL && temp != NULL) {
            int curr_id, studentId, courseId;
            char grade[MAX_GRADE_LENGTH + 1];
            int curr_status;

            while (fscanf(file, "%d %d %d %s %d\n", &curr_id, &studentId, &courseId, 
                         grade, &curr_status) != EOF) {
                if (curr_id == enrollmentId) {
                    fprintf(temp, "%d %d %d %s %d\n", curr_id, studentId, courseId,
                            grade, status);
                } else {
                    fprintf(temp, "%d %d %d %s %d\n", curr_id, studentId, courseId,
                            grade, curr_status);
                }
            }

            fclose(file);
            fclose(temp);
            remove("data/Enrollments.txt");
            rename("data/Enrollments_temp.txt", "data/Enrollments.txt");
        }
    } else {
        printf("Enrollment not found.\n");
    }

    release_lock(4, EXCLUSIVE); // Unlock after updating status
}

// Delete enrollment
void deleteEnrollment(int enrollmentId) {
    acquire_lock(4, EXCLUSIVE); // Lock for deleting an enrollment

    int index = hash(enrollmentId);
    while (enrollmentHashTable[index] != NULL) {
        if (enrollmentHashTable[index]->occupied && enrollmentHashTable[index]->id == enrollmentId) {
            enrollmentHashTable[index]->occupied = 0;
            printf("\nEnrollment deleted successfully!\n");

            // Remove from ID array
            int *found = bsearch(&enrollmentId, enrollmentIdArray, enrollmentCounter, sizeof(int), compareEnrollmentId);
            if (found != NULL) {
                memmove(found, found + 1, (--enrollmentCounter - (found - enrollmentIdArray)) * sizeof(int));
            }

            // Update file
            FILE *file = fopen("data/Enrollments.txt", "r");
            FILE *temp = fopen("data/Enrollments_temp.txt", "w");

            if (file != NULL && temp != NULL) {
                int curr_id, studentId, courseId;
                char grade[MAX_GRADE_LENGTH + 1];
                int status;

                while (fscanf(file, "%d %d %d %s %d\n", &curr_id, &studentId, &courseId, 
                             grade, &status) != EOF) {
                    if (curr_id != enrollmentId) {
                        fprintf(temp, "%d %d %d %s %d\n", curr_id, studentId, courseId,
                                grade, status);
                    }
                }

                fclose(file);
                fclose(temp);
                remove("data/Enrollments.txt");
                rename("data/Enrollments_temp.txt", "data/Enrollments.txt");
            }

            release_lock(4, EXCLUSIVE); // Unlock after deletion
            return;
        }
        index = hash(index + 1);
    }
    printf("Enrollment not found.\n");
    release_lock(4, EXCLUSIVE);
}

// Search functions
Enrollment *searchEnrollmentById(int id) {
    int index = hash(id);
    while (enrollmentHashTable[index] != NULL) {
        if (enrollmentHashTable[index]->occupied && enrollmentHashTable[index]->id == id) {
            Enrollment *result = enrollmentHashTable[index];
            return result;
        }
        index = hash(index + 1);
    }
    return NULL;
}

Enrollment *searchEnrollmentByStudentAndCourse(int studentId, int courseId) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->studentId == studentId &&
            enrollmentHashTable[i]->courseId == courseId) {
            Enrollment *result = enrollmentHashTable[i];
            return result;
        }
    }
    return NULL;
}

void searchEnrollmentsByStudent(int studentId) {
    acquire_lock(4, SHARED); // Acquire a shared lock before reading
    printf("\nEnrollments for Student %d:\n", studentId);
    bool found = false;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->studentId == studentId) {
            showEnrollment(enrollmentHashTable[i]);
            found = true;
        }
    }

    if (!found) {
        printf("No enrollments found for this student.\n");
    }
    release_lock(4, SHARED); // Releasing the lock after reading
}

void searchEnrollmentsByCourse(int courseId) {
    acquire_lock(4, SHARED); // Acquire a shared lock before reading
    printf("\nEnrollments for Course %d:\n", courseId);
    bool found = false;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->courseId == courseId) {
            showEnrollment(enrollmentHashTable[i]);
            found = true;
        }
    }

    if (!found) {
        printf("No enrollments found for this course.\n");
    }
    release_lock(4, SHARED); // Release the lock after reading
}

void *printEnrollment(void *arg) {
    EnrollmentThreadArg *threadArg = (EnrollmentThreadArg *)arg;
    Enrollment *enrollment = threadArg->enrollment;

    acquire_lock(4, SHARED); // Acquire a shared lock before displaying
    printf("Enrollment ID: %d Student ID: %d Course ID: %d Grade: %s Status: %s\n",
       enrollment->id, enrollment->studentId, enrollment->courseId,
       enrollment->grade, getStatusString(enrollment->status));
    release_lock(4, SHARED); // Release the lock after displaying

    return NULL;
}

void showAllEnrollments() {
    acquire_lock(4, SHARED); // Acquire a shared lock before displaying
    printf("\nThere are currently %d enrollment(s) in the database.\n", enrollmentCounter);

    pthread_t threads[enrollmentCounter];
    EnrollmentThreadArg args[enrollmentCounter];
    int threadIndex = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied) {
            args[threadIndex].enrollment = enrollmentHashTable[i];
            pthread_create(&threads[threadIndex], NULL, printEnrollment, &args[threadIndex]);
            threadIndex++;
        }
    }

    for (int i = 0; i < threadIndex; i++) {
        pthread_join(threads[i], NULL);
    }
    release_lock(4, SHARED); // Release the lock after displaying
}

void showEnrollment(Enrollment *enrollment) {
    if (!enrollment) {
        printf("No enrollment data to display.\n");
        return;
    }

    acquire_lock(4, SHARED); // Acquire a shared lock before displaying
    printf("\n*****************\n");
    printf("Enrollment ID: %d\n", enrollment->id);
    printf("Student ID: %d\n", enrollment->studentId);
    printf("Course ID: %d\n", enrollment->courseId);
    printf("Grade: %s\n", enrollment->grade);
    printf("Status: %s\n", getStatusString(enrollment->status));
    release_lock(4, SHARED); // Release the lock after displaying
}


// Menu operations
Enrollment *selectEnrollmentMenu() {
    int choice;
    do {
        printf("\nSelect Enrollment:\n");
        printf("1. Search by ID\n");
        printf("2. Search by Student and Course\n");
        printf("0. Cancel\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        int id, studentId, courseId;
        Enrollment *enrollment = NULL;
        
        switch(choice) {
            case 1:
                printf("Enter Enrollment ID: ");
                scanf("%d", &id);
                getchar();
                enrollment = searchEnrollmentById(id);
                break;
            case 2:
                printf("Enter Student ID: ");
                scanf("%d", &studentId);
                getchar();
                printf("Enter Course ID: ");
                scanf("%d", &courseId);
                getchar();
                enrollment = searchEnrollmentByStudentAndCourse(studentId, courseId);
                break;
            case 0:
                return NULL;
            default:
                printf("Invalid choice.\n");
                return NULL;
        }
        
        if (enrollment) {
            showEnrollment(enrollment);
            return enrollment;
        } else {
            printf("Enrollment not found.\n");
        }
    } while (choice != 0);
    
    return NULL;
}

void insertEnrollmentMenu() {
    Enrollment *enrollment = malloc(sizeof(Enrollment));
    
    printf("\nAdd New Enrollment\n");
    printf("Enter Enrollment ID: ");
    scanf("%d", &enrollment->id);
    getchar();
    
    printf("Enter Student ID: ");
    scanf("%d", &enrollment->studentId);
    getchar();
    
    printf("Enter Course ID: ");
    scanf("%d", &enrollment->courseId);
    getchar();
    
    strcpy(enrollment->grade, ""); // Initialize empty grade
    enrollment->status = ENROLLED; // Initialize status as enrolled
    
    insertEnrollment(enrollment, false);
}

void updateGradeMenu() {
    Enrollment *enrollment = selectEnrollmentMenu();
    if (enrollment == NULL) return;

    char grade[MAX_GRADE_LENGTH + 1];
    printf("Enter new grade (A, B, C, D, or F): ");
    scanf("%s", grade);
    getchar();
    
    updateGrade(enrollment->id, grade);
}

void updateStatusMenu() {
    Enrollment *enrollment = selectEnrollmentMenu();
    if (enrollment == NULL) return;

    printf("Select new status:\n");
    printf("1. Enrolled\n");
    printf("2. Dropped\n");
    printf("3. Completed\n");
    printf("Enter choice: ");
    
    int choice;
    scanf("%d", &choice);
    getchar();
    
    EnrollmentStatus status;
    switch(choice) {
        case 1: status = ENROLLED; break;
        case 2: status = DROPPED; break;
        case 3: status = COMPLETED; break;
        default:
            printf("Invalid choice.\n");
            return;
    }
    
    updateStatus(enrollment->id, status);
}

void deleteEnrollmentMenu() {
    Enrollment *enrollment = selectEnrollmentMenu();
    if (enrollment != NULL) {
        deleteEnrollment(enrollment->id);
    }
}

void enrollmentMenu() {
    int choice;
    do {
        printf("\nEnrollment Operations\n");
        printf("1. Show All Enrollments\n");
        printf("2. Show Enrollment\n");
        printf("3. Add Enrollment\n");
        printf("4. Update Grade\n");
        printf("5. Update Status\n");
        printf("6. Delete Enrollment\n");
        printf("7. Show Course Stats\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        switch(choice) {
            case 1:
                showAllEnrollments();
                break;
            case 2:
                selectEnrollmentMenu();
                break;
            case 3:
                insertEnrollmentMenu();
                break;
            case 4:
                updateGradeMenu();
                break;
            case 5:
                updateStatusMenu();
                break;
            case 6:
                deleteEnrollmentMenu();
                break;
            case 7:
                {
                    int courseId;
                    printf("Enter Course ID: ");
                    scanf("%d", &courseId);
                    getchar();
                    showCourseStats(courseId);
                }
                break;
            case 0:
                break;
            default:
                printf("Invalid choice.\n");
        }
    } while (choice != 0);
}

// Helper functions
const char* getStatusString(EnrollmentStatus status) {
    switch (status) {
        case ENROLLED: return "Enrolled";
        case DROPPED: return "Dropped";
        case COMPLETED: return "Completed";
        default: return "Unknown";
    }
}

bool validateStatus(EnrollmentStatus status) {
    return status >= ENROLLED && status <= COMPLETED;
}

bool validateStudentReference(int studentId) {
    return searchStudentById(studentId) != NULL;
}

bool validateCourseReference(int courseId) {
    return searchCourseById(courseId) != NULL;
}

bool validateGrade(char grade[]) {
    if (grade == NULL || strlen(grade) == 0) {
        return false;
    }
    
    if (strlen(grade) > MAX_GRADE_LENGTH) {
        return false;
    }
    
    // Only allow single letter grades A, B, C, D, F
    char validGrades[] = "ABCDF";
    if (strlen(grade) != 1 || strchr(validGrades, grade[0]) == NULL) {
        return false;
    }
    
    return true;
}

bool isStudentEnrolledInCourse(int studentId, int courseId) {
    return searchEnrollmentByStudentAndCourse(studentId, courseId) != NULL;
}

// Statistics and reports
void showCourseStats(int courseId) {
    acquire_lock(2, SHARED); // Lock the course resource for shared reading
    Course *course = searchCourseById(courseId);
    if (course == NULL) {
        release_lock(2, SHARED); // Release the lock if course not found
        printf("Course not found.\n");
        return;
    }

    int totalEnrolled = 0;
    int totalDropped = 0;
    int totalCompleted = 0;

    acquire_lock(4, SHARED); // Lock the enrollment resource for shared reading
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->courseId == courseId) {
            switch(enrollmentHashTable[i]->status) {
                case ENROLLED: totalEnrolled++; break;
                case DROPPED: totalDropped++; break;
                case COMPLETED: totalCompleted++; break;
            }
        }
    }
    release_lock(4, SHARED); // Release the enrollment lock

    printf("\nCourse Statistics for %s (ID: %d):\n", course->title, courseId);
    printf("Enrolled: %d\n", totalEnrolled);
    printf("Dropped: %d\n", totalDropped);
    printf("Completed: %d\n", totalCompleted);

    release_lock(2, SHARED); // Release the course lock
}

int getEnrollmentCount(int courseId) {
    int count = 0;
    acquire_lock(4, SHARED); // Lock the enrollment resource for shared reading
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->courseId == courseId &&
            enrollmentHashTable[i]->status == ENROLLED) {
            count++;
        }
    }
    release_lock(4, SHARED); // Release the lock after counting
    return count;
}


