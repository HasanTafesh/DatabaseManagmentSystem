#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>

// Common size definitions
#define HASH_TABLE_SIZE 100  
#define NAME_MAPPING_SIZE 100  

extern int hashTableSize; 

// Hash function
int hash(int key);

// Common validation functions for department and instructor since they are used in multiple modules
bool validateDepartmentReference(int departmentId);
bool validateInstructorReference(int instructorId);

#endif // COMMON_H
