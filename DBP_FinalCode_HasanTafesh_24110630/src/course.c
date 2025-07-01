#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include "../include/course.h"
#include "../include/department.h"
#include "../include/instructor.h"
#include "../include/student.h"
#include "../include/enrollment.h"
#include "../include/common.h"
#include "../include/lock_management.h"

// Global variables
Course **courseHashTable = NULL; // Dynamic hash table pointer
CourseTitleIdMapping courseTitleMapping[NAME_MAPPING_SIZE];
int courseMappingCount = 0;
int courseCounter = 0;
int *courseIdArray;

// Comparison functions for sorting
int compareCourseTitle(const void *a, const void *b) {
    return strcmp(((CourseTitleIdMapping *)a)->title, ((CourseTitleIdMapping *)b)->title);
}

int compareCourseId(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

// Initialize courses
void initCourses() {
    // Allocate dynamic hash table
    courseHashTable = calloc(hashTableSize, sizeof(Course *));
    if (courseHashTable == NULL) {
        printf("Memory allocation failed for course hash table.\n");
        exit(EXIT_FAILURE);
    }

    // Allocate ID array
    courseIdArray = malloc(hashTableSize * sizeof(int));
    if (courseIdArray == NULL) {
        printf("Memory allocation failed for course ID array.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < hashTableSize; i++) {
        courseIdArray[i] = -1;
    }

    // Read courses from file
    FILE *file = fopen("data/Courses.txt", "r");
    if (file != NULL) {
        int id, credits, departmentId, instructorId;
        char title[100];

        while (fscanf(file, "%d %s %d %d %d\n", &id, title, &credits,
                     &departmentId, &instructorId) != EOF) {
            Course *course = malloc(sizeof(Course));
            course->id = id;
            strncpy(course->title, title, 100);
            course->credits = credits;
            course->departmentId = departmentId;
            course->instructorId = instructorId;
            course->occupied = 1;

            insertCourse(course, true);
        }
        fclose(file);
    }
}


// Validation functions
bool validateCourseData(Course *course) {
    if (course->credits <= 0) {
        printf("Error: Credits must be positive.\n");
        return false;
    }
    if (searchDepartmentById(course->departmentId) == NULL) {
        printf("Error: Department ID %d does not exist.\n", course->departmentId);
        return false;
    }
    if (searchInstructorById(course->instructorId) == NULL) {
        printf("Error: Instructor ID %d does not exist.\n", course->instructorId);
        return false;
    }
    return true;
}

// Insert new course
void insertCourse(Course *course, bool isInit) {
    acquire_lock(2, EXCLUSIVE); // Lock for exclusive access to the course data

    // Validate course data if not during initialization
    if (!isInit && !validateCourseData(course)) {
        release_lock(2, EXCLUSIVE);
        return;
    }

    // Check for duplicate ID
    if (bsearch(&(course->id), courseIdArray, courseCounter, sizeof(int), compareCourseId) != NULL) {
        printf("\nFailed to add course. A course with ID %d already exists.\n", course->id);
        release_lock(2, EXCLUSIVE);
        return;
    }

    // Resize the hash table if load factor exceeds 0.75
    if ((float)courseCounter / hashTableSize > 0.75) { // Load factor > 0.75
        int newSize = hashTableSize * 2; // Double the size
        Course **newHashTable = calloc(newSize, sizeof(Course *));
        if (newHashTable == NULL) {
            printf("Error: Memory allocation failed during hash table resizing.\n");
            release_lock(2, EXCLUSIVE);
            return;
        }

        // Rehash existing courses into the new table
        for (int i = 0; i < hashTableSize; i++) {
            if (courseHashTable[i] != NULL && courseHashTable[i]->occupied) {
                int newIndex = hash(courseHashTable[i]->id) % newSize;
                while (newHashTable[newIndex] != NULL) {
                    newIndex = (newIndex + 1) % newSize;
                }
                newHashTable[newIndex] = courseHashTable[i];
            }
        }

        free(courseHashTable);
        courseHashTable = newHashTable;
        hashTableSize = newSize;
    }

    // Resize the ID array if necessary
    if (courseCounter == hashTableSize) {
        int *newIdArray = realloc(courseIdArray, hashTableSize * 2 * sizeof(int));
        if (newIdArray == NULL) {
            printf("Error: Memory allocation failed for course ID array resizing.\n");
            release_lock(2, EXCLUSIVE);
            return;
        }
        courseIdArray = newIdArray;
    }

    // Add ID to array and sort
    courseIdArray[courseCounter++] = course->id;
    qsort(courseIdArray, courseCounter, sizeof(int), compareCourseId);

    // Insert course into hash table
    int index = hash(course->id) % hashTableSize;
    int originalIndex = index;
    while (courseHashTable[index] != NULL) {
        if (!courseHashTable[index]->occupied) { 
            free(courseHashTable[index]);
            break;
        }
        index = (index + 1) % hashTableSize; 
        if (index == originalIndex) { 
            printf("Error: Hash table is full. Cannot insert course.\n");
            release_lock(2, EXCLUSIVE);
            return;
        }
    }
    courseHashTable[index] = course;
    courseHashTable[index]->occupied = 1;

    // Add to title mapping array
    if (courseMappingCount < NAME_MAPPING_SIZE) {
        strncpy(courseTitleMapping[courseMappingCount].title, course->title, 50);
        courseTitleMapping[courseMappingCount].id = course->id;
        courseMappingCount++;
        qsort(courseTitleMapping, courseMappingCount, sizeof(CourseTitleIdMapping), compareCourseTitle);
    }

    // Store course in file if not during initialization
    if (!isInit) {
        FILE *file = fopen("data/Courses.txt", "a");
        if (file) {
            fprintf(file, "%d %s %d %d %d\n", 
                    course->id, course->title, course->credits,
                    course->departmentId, course->instructorId);
            fflush(file);
            fclose(file);
            printf("\nCourse added successfully!\n");
        } else {
            perror("Failed to write to Courses.txt");
        }
    }

    release_lock(2, EXCLUSIVE); // Release the lock after insertion
}



// Update course
void updateCourse(int id, int instructorId) {

    acquire_lock(2, EXCLUSIVE); // Lock for updating a course

    Course *course = searchCourseById(id);
    if (course != NULL) {
        if (!validateInstructorReference(instructorId)) {
            printf("Error: Invalid instructor ID\n");
            release_lock(2, EXCLUSIVE); // Unlock if validation fails
            return;
        }

        course->instructorId = instructorId;
        printf("\nInstructor updated successfully.\n");

        // Update file
        FILE *file = fopen("data/Courses.txt", "r");
        FILE *temp = fopen("data/Courses_temp.txt", "w");

        if (file != NULL && temp != NULL) {
            int curr_id, credits, departmentId, curr_instructorId;
            char title[100];

            while (fscanf(file, "%d %s %d %d %d\n", &curr_id, title, &credits,
                          &departmentId, &curr_instructorId) != EOF) {
                if (curr_id == id) {
                    fprintf(temp, "%d %s %d %d %d\n", id, title, credits,
                            departmentId, instructorId);
                } else {
                    fprintf(temp, "%d %s %d %d %d\n", curr_id, title, credits,
                            departmentId, curr_instructorId);
                }
            }

            fclose(file);
            fclose(temp);
            remove("data/Courses.txt");
            rename("data/Courses_temp.txt", "data/Courses.txt");
        }
    } else {
        printf("Course not found.\n");
    }

    release_lock(2, EXCLUSIVE); // Unlock after updating
}

// Delete course
void deleteCourse(int id) {
    acquire_lock(2, EXCLUSIVE); // Lock for deleting a course

    int index = hash(id);
    while (courseHashTable[index] != NULL) {
        if (courseHashTable[index]->occupied && courseHashTable[index]->id == id) {
            // Check for existing enrollments
            for (int i = 0; i < HASH_TABLE_SIZE; i++) {
                if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
                    enrollmentHashTable[i]->courseId == id) {
                    printf("Error: Cannot delete course - Students are enrolled\n");
                    release_lock(2, EXCLUSIVE); // Unlock if there are enrollments
                    return;
                }
            }

            courseHashTable[index]->occupied = 0;
            printf("\nCourse %s deleted successfully!\n", courseHashTable[index]->title);

            // Remove from mapping array
            for (int i = 0; i < courseMappingCount; i++) {
                if (courseTitleMapping[i].id == id) {
                    for (int j = i; j < courseMappingCount - 1; j++) {
                        courseTitleMapping[j] = courseTitleMapping[j + 1];
                    }
                    courseMappingCount--;
                    break;
                }
            }

            // Remove from ID array
            int *found = bsearch(&id, courseIdArray, courseCounter, sizeof(int), compareCourseId);
            if (found != NULL) {
                memmove(found, found + 1, (--courseCounter - (found - courseIdArray)) * sizeof(int));
            }

            // Update file
            FILE *file = fopen("data/Courses.txt", "r");
            FILE *temp = fopen("data/Courses_temp.txt", "w");

            if (file != NULL && temp != NULL) {
                int curr_id, credits, departmentId, instructorId;
                char title[100];

                while (fscanf(file, "%d %s %d %d %d\n", &curr_id, title, &credits,
                              &departmentId, &instructorId) != EOF) {
                    if (curr_id != id) {
                        fprintf(temp, "%d %s %d %d %d\n", curr_id, title, credits,
                                departmentId, instructorId);
                    }
                }

                fclose(file);
                fclose(temp);
                remove("data/Courses.txt");
                rename("data/Courses_temp.txt", "data/Courses.txt");
            }

            release_lock(2, EXCLUSIVE); // Unlock after deletion
            return;
        }
        index = hash(index + 1);
    }
    printf("Course not found.\n");
    release_lock(2, EXCLUSIVE);
}

// Search functions
Course *searchCourseById(int id) {
    int index = hash(id);
    while (courseHashTable[index] != NULL) {
        if (courseHashTable[index]->occupied && courseHashTable[index]->id == id) {
            Course *result = courseHashTable[index];
            return result;
        }
        index = hash(index + 1);
    }
    return NULL;
}


Course *searchCourseByTitle(char title[]) {
    for (int i = 0; i < courseMappingCount; i++) {
        if (strcmp(courseTitleMapping[i].title, title) == 0) {
            Course *result = searchCourseById(courseTitleMapping[i].id);
            return result;
        }
    }
    return NULL;
}


void searchCoursesByDepartment(int departmentId) {
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
}

// Display functions
void *printCourse(void *arg) {
    CourseThreadArg *threadArg = (CourseThreadArg *)arg;
    Course *course = threadArg->course;

    printf("Course ID: %d Title: %s Credits: %d Department ID: %d Instructor ID: %d\n",
           course->id, course->title, course->credits, course->departmentId, course->instructorId);

    return NULL;
}

void showAllCourses() {
    acquire_lock(2, SHARED); // Acquire a shared lock before displaying
    printf("\nThere are currently %d course(s) in the database.\n", courseCounter);

    pthread_t threads[courseCounter];
    CourseThreadArg args[courseCounter];
    int threadIndex = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (courseHashTable[i] != NULL && courseHashTable[i]->occupied) {
            args[threadIndex].course = courseHashTable[i];
            pthread_create(&threads[threadIndex], NULL, printCourse, &args[threadIndex]);
            threadIndex++;
        }
    }

    for (int i = 0; i < threadIndex; i++) {
        pthread_join(threads[i], NULL);
    }
    release_lock(2, SHARED); // Release the lock after displaying
}


void showCourse(Course *course) {
    if (!course) {
        printf("No course data to display.\n");
        return;
    }

    printf("\n*********************************************\n");
    printf("Course ID: %d\n", course->id);
    printf("Title: %s\n", course->title);
    printf("Credits: %d\n", course->credits);
    printf("Department ID: %d\n", course->departmentId);
    printf("Instructor ID: %d\n", course->instructorId);
}


// Menu operations
Course *selectCourseMenu() {
    int choice;
    do {
        printf("\nSelect Course:\n");
        printf("1. Search by ID\n");
        printf("2. Search by Title\n");
        printf("0. Cancel\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        int id;
        char title[100];
        Course *course = NULL;
        
        switch(choice) {
            case 1:
                printf("Enter Course ID: ");
                scanf("%d", &id);
                getchar();
                course = searchCourseById(id);
                break;
            case 2:
                printf("Enter Course Title: ");
                fgets(title, sizeof(title), stdin);
                title[strcspn(title, "\n")] = 0;
                course = searchCourseByTitle(title);
                break;
            case 0:
                return NULL;
            default:
                printf("Invalid choice.\n");
                return NULL;
        }
        
        if (course) {
            showCourse(course);
            return course;
        } else {
            printf("Course not found.\n");
        }
    } while (choice != 0);
    
    return NULL;
}

void insertCourseMenu() {
    Course *course = malloc(sizeof(Course));
    
    printf("\nAdd New Course\n");
    printf("Enter Course ID: ");
    scanf("%d", &course->id);
    getchar();
    
    printf("Enter Course Title: ");
    fgets(course->title, sizeof(course->title), stdin);
    course->title[strcspn(course->title, "\n")] = 0;
    
    printf("Enter Credits: ");
    scanf("%d", &course->credits);
    getchar();
    
    printf("Enter Department ID: ");
    scanf("%d", &course->departmentId);
    getchar();
    
    printf("Enter Instructor ID: ");
    scanf("%d", &course->instructorId);
    getchar();
    
    insertCourse(course, false);
}

void updateCourseMenu() {
    Course *course = selectCourseMenu();
    if (course == NULL) return;

    int instructorId;
    printf("Enter new instructor ID: ");
    scanf("%d", &instructorId);
    getchar();
    
    updateCourse(course->id, instructorId);
}

void deleteCourseMenu() {
    Course *course = selectCourseMenu();
    if (course != NULL) {
        deleteCourse(course->id);
    }
}

void courseMenu() {
    int choice;
    do {
        printf("\nCourse Operations\n");
        printf("1. Show All Courses\n");
        printf("2. Show Course\n");
        printf("3. Add Course\n");
        printf("4. Update Course\n");
        printf("5. Delete Course\n");
        printf("6. Show Course Details\n");
        printf("7. Show Enrolled Students\n");
        printf("0. Back to Main Menu\n");
        printf("Enter choice: ");
        
        scanf("%d", &choice);
        getchar();
        
        switch(choice) {
            case 1:
                showAllCourses();
                break;
            case 2:
                selectCourseMenu();
                break;
            case 3:
                insertCourseMenu();
                break;
            case 4:
                updateCourseMenu();
                break;
            case 5:
                deleteCourseMenu();
                break;
            case 6:
                {
                    Course *course = selectCourseMenu();
                    if (course) showCourseDetails(course->id);
                }
                break;
            case 7:
                {
                    Course *course = selectCourseMenu();
                    if (course) showEnrolledStudents(course->id);
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
void showEnrolledStudents(int courseId) {
    Course *course = searchCourseById(courseId);
    if (course == NULL) {
        printf("Error: Course not found\n");
        return;
    }
    
    printf("\nEnrolled students for course %d - %s:\n", courseId, course->title);
    bool found = false;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (enrollmentHashTable[i] != NULL && enrollmentHashTable[i]->occupied &&
            enrollmentHashTable[i]->courseId == courseId) {
            Student *student = searchStudentById(enrollmentHashTable[i]->studentId);
            if (student != NULL) {
                printf("Student ID: %d\n", student->id);
                printf("Name: %s %s\n", student->firstName, student->lastName);
                printf("Status: %s\n", getStatusString(enrollmentHashTable[i]->status));
                printf("Grade: %s\n", enrollmentHashTable[i]->grade);
                printf("-------------------------\n");
                found = true;
            }
        }
    }
    if (!found) {
        printf("No students enrolled in this course.\n");
    }

}

void showCourseDetails(int courseId) {
    Course *course = searchCourseById(courseId);
    if (course == NULL) {
        printf("Error: Course not found\n");
        return;
    }
    
    printf("\nDetailed Course Information:\n");
    showCourse(course);
    
    Department *dept = searchDepartmentById(course->departmentId);
    if (dept) {
        printf("Department: %s\n", dept->name);
    }
    
    Instructor *inst = searchInstructorById(course->instructorId);
    if (inst) {
        printf("Instructor: %s %s\n", inst->firstName, inst->lastName);
    }
    
    printf("\nEnrolled Students:\n");
    showEnrolledStudents(courseId);

}
