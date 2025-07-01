#ifndef INSTRUCTOR_H
#define INSTRUCTOR_H

#include <stdbool.h>
#include "department.h"
#include "common.h"

#define NAME_MAPPING_SIZE 100
#define MAX_PHONE_NUMBERS 3

// Structs for instructor and phone numbers
typedef struct InstructorPhoneNumber {
    int id;                     // Primary key
    char phone[15];            // Phone number
    int instructorId;          // Foreign key to Instructor
} InstructorPhoneNumber;

typedef struct Instructor {
    int id;                     // Primary key
    char firstName[50];         // First name
    char lastName[50];          // Last name
    char email[100];           // Email address
    int departmentId;          // Foreign key to Department
    int occupied;              // Flag for hash table slot occupation
} Instructor;

typedef struct {
    char firstName[50];         // For name-based search
    char lastName[50];          // Last name
    int id;                    // Corresponding instructor ID
} InstructorNameIdMapping;

typedef struct {
    Instructor *instructor;     // For thread operations
} InstructorThreadArg;



extern Instructor **instructorHashTable; // Declare the instructor hash table
extern InstructorNameIdMapping instructorNameMapping[NAME_MAPPING_SIZE];
extern int instructorMappingCount;
extern int instructorCounter;
extern int *instructorIdArray;

// Core operations
void initInstructors();
void storeInstructor(Instructor *inst);
void insertInstructor(Instructor *inst, bool isInit);
void updateInstructor(int id, char *email);
void deleteInstructor(int id);

// Phone number operations
void addInstructorPhoneNumber(int instructorId, char phone[]);
void removeInstructorPhoneNumber(int phoneNumberId);
void showInstructorPhoneNumbers(int instructorId);
void storeInstructorPhoneNumber(InstructorPhoneNumber *phone);

// Search operations
Instructor *searchInstructorById(int id);
Instructor *searchInstructorByName(char firstName[], char lastName[]);
Instructor *searchInstructorByEmail(char email[]);
void searchInstructorsByDepartment(int departmentId);

// Display operations
void showInstructor(Instructor *inst);
void showAllInstructors();
void showInstructorCourses(int instructorId);

// Menu operations
void instructorMenu();
Instructor *selectInstructorMenu();
void insertInstructorMenu();
void updateInstructorMenu();
void deleteInstructorMenu();
void manageInstructorPhoneNumbersMenu(int instructorId);


#endif /* INSTRUCTOR_H */
