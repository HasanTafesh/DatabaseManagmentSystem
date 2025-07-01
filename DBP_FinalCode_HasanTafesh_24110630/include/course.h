#ifndef COURSE_H
#define COURSE_H

#include <stdbool.h>
#include "department.h"
#include "instructor.h"
#include "common.h"

#define HASH_TABLE_SIZE 100
#define NAME_MAPPING_SIZE 100

typedef struct Course {
    int id;                     
    char title[100];           
    int credits;               
    int departmentId;          
    int instructorId;          
    int occupied;              // Flag for hash table slot occupation
} Course;

typedef struct {
    char title[100];           // For searcheaing using the title
    int id;                    
} CourseTitleIdMapping;

typedef struct {
    Course *course;            // For thread operations
} CourseThreadArg;

// Hash table and mapping declarations
extern Course **courseHashTable;
extern CourseTitleIdMapping courseTitleMapping[NAME_MAPPING_SIZE];
extern int courseMappingCount;
extern int courseCounter;
extern int *courseIdArray;

// Core operations
void initCourses();
void insertCourse(Course *course, bool isInit);
void updateCourse(int id, int instructorId);
void deleteCourse(int id);

// Search operations
Course *searchCourseById(int id);
Course *searchCourseByTitle(char title[]);
void searchCoursesByDepartment(int departmentId);

// Display operations
void showCourse(Course *course);
void showAllCourses();
void showEnrolledStudents(int courseId);
void showCourseDetails(int courseId);

// Menu operations
Course *selectCourseMenu();
void insertCourseMenu();
void updateCourseMenu();
void deleteCourseMenu();
void courseMenu();

// Validation
bool validateCourseData(Course *course);
bool validateDepartmentReference(int departmentId);
bool validateInstructorReference(int instructorId);



#endif /* COURSE_H */
