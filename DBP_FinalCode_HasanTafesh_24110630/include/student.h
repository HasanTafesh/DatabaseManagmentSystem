#ifndef STUDENT_H
#define STUDENT_H

#include <stdbool.h>
#include "department.h"
#include "common.h"


#define HASH_TABLE_SIZE 100  
#define NAME_MAPPING_SIZE 100

typedef struct Student {
    int id;                     // Primary key
    char firstName[50];         // First name
    char lastName[50];          // Last name
    char email[100];           // Email address
    char phone[15];            // Phone number
    int departmentId;          // Foreign key to Department
    int occupied;              // Flag for hash table slot occupation
} Student;

typedef struct {
    char firstName[50];         // For name-based search
    char lastName[50];
    int id;                    // Corresponding student ID
} StudentNameIdMapping;

typedef struct {
    Student *student;          // For thread operations
} StudentThreadArg;

// Hash table and mapping declarations
extern Student **studentHashTable;  // Pointer for dynamic hash table
extern StudentNameIdMapping studentNameMapping[NAME_MAPPING_SIZE];
extern int studentMappingCount;
extern int studentCounter;
extern int *studentIdArray;

// Core operations
void initStudents();
void storeStudent(Student *student);
void insertStudent(Student *student, bool isInit);
void updateStudent(int id, char *phone);
void deleteStudent(int id);

// Search operations
Student *searchStudentById(int id);
Student *searchStudentByName(char firstName[], char lastName[]);
Student *searchStudentByEmail(char email[]);
Student *searchStudentByPhone(char phone[]);
void searchStudentsByDepartment(int departmentId);

// Display operations
void showStudent(Student *student);
void showAllStudents();
void showStudentCourses(int studentId);
void showStudentGrades(int studentId);

// Menu operations
Student *selectStudentMenu();
void insertStudentMenu();
void updateStudentMenu();
void deleteStudentMenu();
void studentMenu();

// Course enrollment operations
void enrollStudentInCourse(int studentId, int courseId);

#endif /* STUDENT_H */
