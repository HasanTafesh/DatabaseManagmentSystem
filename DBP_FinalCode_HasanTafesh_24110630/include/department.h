#ifndef DEPARTMENT_H
#define DEPARTMENT_H

#include <stdbool.h>
#include "common.h"

#define HASH_TABLE_SIZE 100
#define NAME_MAPPING_SIZE 100

typedef struct Department {
    int id;                     
    char name[100];             
    char phone[15];             
    int occupied;               // Flag for hash table slot occupation
} Department;

typedef struct {
    char name[100];             // For search using name 
    int id;                    
} DepartmentNameIdMapping;

typedef struct {
    Department *department;     // For thread operations
} DepartmentThreadArg;

// Hash table and mapping declarations
extern Department **departmentHashTable;
extern DepartmentNameIdMapping departmentNameMapping[NAME_MAPPING_SIZE];
extern int departmentMappingCount;
extern int departmentCounter;
extern int *departmentIdArray;

// Core operations
void initDepartments();
void storeDepartment(Department *dept);
void insertDepartment(Department *dept, bool isInit);
void updateDepartment(int id, char *phone);
void deleteDepartment(int id);

// Search operations
Department *searchDepartmentById(int id);
Department *searchDepartmentByName(char name[]);
Department *searchDepartmentByPhone(char phone[]);

// Display operations
void showDepartment(Department *dept);
void showAllDepartments();
void showInstructorsInDepartment(int departmentId);
void showCoursesInDepartment(int departmentId);
void showStudentsInDepartment(int departmentId);

// Menu operations
Department *selectDepartmentMenu();
void insertDepartmentMenu();
void updateDepartmentMenu();
void deleteDepartmentMenu();
void departmentMenu();

// Validation
bool validateDepartmentData(Department *dept);

#endif /* DEPARTMENT_H */
