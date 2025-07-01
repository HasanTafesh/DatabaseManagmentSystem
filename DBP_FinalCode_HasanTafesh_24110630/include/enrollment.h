#ifndef ENROLLMENT_H
#define ENROLLMENT_H

#include <stdbool.h>
#include "student.h"
#include "course.h"
#include "common.h"

#define HASH_TABLE_SIZE 100
#define MAX_GRADE_LENGTH 2
#define MAX_STATUS_LENGTH 10

typedef enum EnrollmentStatus {
    ENROLLED,
    DROPPED,
    COMPLETED
} EnrollmentStatus;

typedef struct Enrollment {
    int id;                   
    int studentId;              
    int courseId;               
    char grade[MAX_GRADE_LENGTH + 1];  // Student's grade ("A", "B+")
    EnrollmentStatus status;    // Enrollment status (Enrolled, Dropped, Completed)
    int occupied;               // Flag for hash table slot occupation
} Enrollment;

typedef struct {
    Enrollment *enrollment;     // For thread operations
} EnrollmentThreadArg;

// Hash table and array declarations
extern Enrollment **enrollmentHashTable; 
extern int enrollmentCounter;
extern int *enrollmentIdArray;

// Core operations
void initEnrollments();
void storeEnrollment(Enrollment *enrollment);
void insertEnrollment(Enrollment *enrollment, bool isInit);
void updateGrade(int enrollmentId, char grade[]);
void updateStatus(int enrollmentId, EnrollmentStatus status);
void deleteEnrollment(int enrollmentId);

// Search operations
Enrollment *searchEnrollmentById(int id);
Enrollment *searchEnrollmentByStudentAndCourse(int studentId, int courseId);
void searchEnrollmentsByStudent(int studentId);
void searchEnrollmentsByCourse(int courseId);

// Display operations
void showEnrollment(Enrollment *enrollment);
void showAllEnrollments();
void showStudentEnrollments(int studentId);
void showCourseEnrollments(int courseId);

// Menu operations
Enrollment *selectEnrollmentMenu();
void insertEnrollmentMenu();
void updateGradeMenu();
void updateStatusMenu();
void deleteEnrollmentMenu();
void enrollmentMenu();

// Helper functions
const char* getStatusString(EnrollmentStatus status);
bool validateStatus(EnrollmentStatus status);

// Validation
bool validateEnrollmentData(Enrollment *enrollment);
bool validateStudentReference(int studentId);
bool validateCourseReference(int courseId);
bool validateGrade(char grade[]);
bool isStudentEnrolledInCourse(int studentId, int courseId);

// Statistics and reports
void showCourseStats(int courseId);
int getEnrollmentCount(int courseId);

#endif /* ENROLLMENT_H */
