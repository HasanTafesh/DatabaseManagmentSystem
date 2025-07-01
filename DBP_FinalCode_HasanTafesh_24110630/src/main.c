#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include "../include/department.h"
#include "../include/instructor.h"
#include "../include/student.h"
#include "../include/course.h"
#include "../include/enrollment.h"
#include "../include/lock_management.h"

#ifdef _WIN32
#include <direct.h>  // to make directory  
#else
#endif


// Function prototypes
void showMainMenu();

// Main menu options
void showMainMenu() {
    printf("\n=== University Database Management System ===\n");
    printf("1. Department Operations\n");
    printf("2. Instructor Operations\n");
    printf("3. Student Operations\n");
    printf("4. Course Operations\n");
    printf("5. Enrollment Operations\n");
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

//concurrency test
// void* insertStudentThread(void* arg) {
//     int id = *(int*)arg;
//     Student *student = malloc(sizeof(Student));
//     student->id = id;
//     strcpy(student->firstName, "ThreadStudent");
//     strcpy(student->lastName, "LastName");
//     strcpy(student->email, "thread@example.com");
//     strcpy(student->phone, "123456789");
//     student->departmentId = 1;
//     insertStudent(student, false);
//     return NULL;
// }

int main(int argc, char *argv[]) {
    // Initializing all the modules
    printf("Initializing University DBMS...\n");

    // Initialize the lock table for managing concurrency control
    initialize_lock_table();

    // Create data directory if it doesn't exist
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0777);
#endif

    // Initialize all modules (which will load data from files)

    initDepartments();
    printf("Initializing Departments\n");
    initInstructors();
    printf("Initializing Instructors\n");
    initStudents();
    printf("Initializing Students\n");
    initCourses();
    printf("Initializing Courses\n");
    initEnrollments();
    printf("Initializing Enrollments\n");

    //Concurrency test
    // pthread_t thread1, thread2;
    // int id1 = 90, id2 = 91;

    // // Initialize the lock table
    // initialize_lock_table();

    // // Create two threads to insert students
    // pthread_create(&thread1, NULL, insertStudentThread, &id1);
    // pthread_create(&thread2, NULL, insertStudentThread, &id2);

    // // Wait for threads to finish
    // pthread_join(thread1, NULL);
    // pthread_join(thread2, NULL);

    // // Verify the results
    // showAllStudents();

    int choice;
    do {
        showMainMenu();
        scanf("%d", &choice);
        getchar(); // Clear newline
        
        switch(choice) {
            case 1:
                departmentMenu();
                break;
            case 2:
                instructorMenu();
                break;
            case 3:
                studentMenu();
                break;
            case 4:
                courseMenu();
                break;
            case 5:
                enrollmentMenu();
                break;
            case 0:
                printf("\nThank you for using the University DBMS!\n");
                exit(0);
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while (1);
    
    return 0;
}
