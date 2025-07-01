#include <stdio.h>
#include "../include/common.h"
#include "../include/department.h"
#include "../include/instructor.h"

int hashTableSize = HASH_TABLE_SIZE;

int hash(int key) {
    return key % HASH_TABLE_SIZE;
}

bool validateDepartmentReference(int departmentId) {
    Department *dept = searchDepartmentById(departmentId);
    if (!dept) {
        printf("Error: Department with ID %d does not exist.\n", departmentId);
        return false;
    }
    return true;
}

bool validateInstructorReference(int instructorId) {
    Instructor *inst = searchInstructorById(instructorId);
    if (!inst) {
        printf("Error: Instructor with ID %d does not exist.\n", instructorId);
        return false;
    }
    return true;
}
