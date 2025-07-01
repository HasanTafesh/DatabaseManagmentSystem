#include "../include/instructor.h"
#include "../include/department.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include "../include/lock_management.h"
#include "../include/common.h"

// Global variables
Instructor **instructorHashTable = NULL; // Dynamic hash table pointer
InstructorNameIdMapping instructorNameIdMapping[NAME_MAPPING_SIZE];
InstructorPhoneNumber *instructorPhoneNumbers = NULL; // Dynamic phone number array
int *instructorIdArray = NULL;
int instructorMappingCount = 0;
int instructorCounter = 0;
int nextPhoneNumberId = 1;

// Initialize instructors
void initInstructors() {
    // Allocate memory for the hash table
    instructorHashTable = calloc(hashTableSize, sizeof(Instructor *));
    if (instructorHashTable == NULL) {
        printf("Memory allocation failed for instructor hash table.\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for phone numbers
    instructorPhoneNumbers = malloc(hashTableSize * MAX_PHONE_NUMBERS * sizeof(InstructorPhoneNumber));
    if (instructorPhoneNumbers == NULL) {
        printf("Memory allocation failed for instructor phone numbers.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < hashTableSize * MAX_PHONE_NUMBERS; i++) {
        instructorPhoneNumbers[i].id = 0;
        instructorPhoneNumbers[i].instructorId = 0;
    }

    // Allocate memory for the ID array
    instructorIdArray = malloc(hashTableSize * sizeof(int));
    if (instructorIdArray == NULL) {
        printf("Memory allocation failed for instructor ID array.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < hashTableSize; i++) {
        instructorIdArray[i] = -1;
    }

    // Load instructors from file
    FILE *file = fopen("data/Instructors.txt", "r");
    if (file != NULL) {
        int id, departmentId;
        char firstName[50], lastName[50], email[100];
        while (fscanf(file, "%d %s %s %s %d\n", &id, firstName, lastName, email, &departmentId) != EOF) {
            Instructor *inst = malloc(sizeof(Instructor));
            inst->id = id;
            strncpy(inst->firstName, firstName, 50);
            strncpy(inst->lastName, lastName, 50);
            strncpy(inst->email, email, 100);
            inst->departmentId = departmentId;
            inst->occupied = 1;
            insertInstructor(inst, true);
        }
        fclose(file);
    }

    // Load phone numbers from file
    file = fopen("data/instructor_phones.txt", "r");
    if (file != NULL) {
        int id, instructorId;
        char phone[15];
        while (fscanf(file, "%d %d %s\n", &id, &instructorId, phone) != EOF) {
            for (int i = 0; i < hashTableSize * MAX_PHONE_NUMBERS; i++) {
                if (instructorPhoneNumbers[i].id == 0) {
                    instructorPhoneNumbers[i].id = id;
                    instructorPhoneNumbers[i].instructorId = instructorId;
                    strncpy(instructorPhoneNumbers[i].phone, phone, 15);
                    if (id >= nextPhoneNumberId) nextPhoneNumberId = id + 1;
                    break;
                }
            }
        }
        fclose(file);
    }
}



int compareInstructorName(const void *a, const void *b)
{
    return strcmp(((InstructorNameIdMapping *)a)->firstName,
                 ((InstructorNameIdMapping *)b)->firstName);
}

int compareInstructorId(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

void storeInstructor(Instructor *inst)
{
    FILE *file = fopen("data/Instructors.txt", "a");
    if (file != NULL)
    {
        fprintf(file, "%d %s %s %s %d\n", 
                inst->id, inst->firstName, inst->lastName,
                inst->email, inst->departmentId);
        fflush(file);
        fclose(file);
    }
    else
    {
        perror("Failed to write to file.");
    }
}

void storeInstructorPhoneNumber(InstructorPhoneNumber *phone)
{
    FILE *file = fopen("data/instructor_phones.txt", "a");
    if (file != NULL)
    {
        fprintf(file, "%d %d %s\n", 
                phone->id, phone->instructorId, phone->phone);
        fflush(file);
        fclose(file);
    }
    else
    {
        perror("Failed to write phone number to file.");
    }
}



// Insert instructor
void insertInstructor(Instructor *inst, bool isInit) {
    acquire_lock(5, EXCLUSIVE);

    // Validate department reference
    if (!isInit && searchDepartmentById(inst->departmentId) == NULL) {
        printf("Error: Department %d does not exist.\n", inst->departmentId);
        release_lock(5, EXCLUSIVE);
        return;
    }

    // Check for duplicate ID
    if (bsearch(&(inst->id), instructorIdArray, instructorCounter, sizeof(int), compareInstructorId) != NULL) {
        printf("Error: An instructor with ID %d already exists.\n", inst->id);
        release_lock(5, EXCLUSIVE);
        return;
    }

    // Resize hash table if load factor exceeds threshold
    if ((float)instructorCounter / hashTableSize > 0.75) {
        int newSize = hashTableSize * 2;
        Instructor **newTable = calloc(newSize, sizeof(Instructor *));
        if (!newTable) {
            printf("Error: Memory allocation failed during hash table resizing.\n");
            release_lock(5, EXCLUSIVE);
            return;
        }

        for (int i = 0; i < hashTableSize; i++) {
            if (instructorHashTable[i] && instructorHashTable[i]->occupied) {
                int newIndex = instructorHashTable[i]->id % newSize;
                while (newTable[newIndex]) newIndex = (newIndex + 1) % newSize;
                newTable[newIndex] = instructorHashTable[i];
            }
        }

        free(instructorHashTable);
        instructorHashTable = newTable;
        hashTableSize = newSize;
    }

    // Insert into hash table
    int index = hash(inst->id) % hashTableSize;
    int originalIndex = index;
    while (instructorHashTable[index] != NULL) {
        if (instructorHashTable[index]->occupied == 0) {
            free(instructorHashTable[index]);
            instructorHashTable[index] = inst;
            instructorHashTable[index]->occupied = 1;
            break;
        }
        index = (index + 1) % hashTableSize;
        if (index == originalIndex) {
            printf("Error: Hash table is full.\n");
            release_lock(5, EXCLUSIVE);
            return;
        }
    }
    if (instructorHashTable[index] == NULL) {
        instructorHashTable[index] = inst;
        instructorHashTable[index]->occupied = 1;
    }

    if (instructorCounter == HASH_TABLE_SIZE) {
        int *newIdArray = realloc(instructorIdArray, HASH_TABLE_SIZE * 2 * sizeof(int));
        if (newIdArray == NULL) {
            printf("Error: Memory allocation failed for instructorIdArray resizing.\n");
            release_lock(5, EXCLUSIVE);
            return;
        }
        instructorIdArray = newIdArray;
    }

    // Add ID to array and sort
    instructorIdArray[instructorCounter++] = inst->id;
    qsort(instructorIdArray, instructorCounter, sizeof(int), compareInstructorId);

    // Add to name mapping
    if (instructorMappingCount < NAME_MAPPING_SIZE) {
        strncpy(instructorNameIdMapping[instructorMappingCount].firstName, inst->firstName, 50);
        strncpy(instructorNameIdMapping[instructorMappingCount].lastName, inst->lastName, 50);
        instructorNameIdMapping[instructorMappingCount].id = inst->id;
        instructorMappingCount++;
        qsort(instructorNameIdMapping, instructorMappingCount, sizeof(InstructorNameIdMapping), compareInstructorName);
    }

    // Store to file
    if (!isInit) storeInstructor(inst);

    release_lock(5, EXCLUSIVE);
}

// Delete instructor
void deleteInstructor(int id) {
    acquire_lock(5, EXCLUSIVE);

    int index = hash(id);
    while (instructorHashTable[index] != NULL) {
        if (instructorHashTable[index]->occupied && instructorHashTable[index]->id == id) {
            for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++) {
                if (instructorPhoneNumbers[i].instructorId == id) {
                    instructorPhoneNumbers[i].id = 0;
                    instructorPhoneNumbers[i].instructorId = 0;
                }
            }

            instructorHashTable[index]->occupied = 0;
            printf("\nInstructor %s deleted successfully!\n", instructorHashTable[index]->firstName);

            for (int i = 0; i < instructorMappingCount; i++) {
                if (instructorNameIdMapping[i].id == id) {
                    for (int j = i; j < instructorMappingCount - 1; j++) {
                        instructorNameIdMapping[j] = instructorNameIdMapping[j + 1];
                    }
                    instructorMappingCount--;
                    break;
                }
            }

            int *found = bsearch(&id, instructorIdArray, instructorCounter, sizeof(int), compareInstructorId);
            if (found != NULL) {
                memmove(found, found + 1, (--instructorCounter - (found - instructorIdArray)) * sizeof(int));
            }
            release_lock(5, EXCLUSIVE);
            return;
        }
        index = hash(index + 1);
    }

    printf("Instructor not found.\n");
    release_lock(5, EXCLUSIVE);
}

void updateInstructor(int id, char *email) {
    acquire_lock(5, EXCLUSIVE);

    Instructor *inst = searchInstructorById(id);
    if (inst != NULL) {
        // Check for unique email
        Instructor *existingInst = searchInstructorByEmail(email);
        if (existingInst != NULL && existingInst->id != id) {
            printf("Error: This email is already registered to another instructor.\n");
            release_lock(5, EXCLUSIVE);
            return;
        }

        strncpy(inst->email, email, 100);
        printf("\nEmail updated successfully.\n");

        // Update email in the file
        FILE *file = fopen("data/Instructors.txt", "r");
        FILE *temp = fopen("data/Instructors_temp.txt", "w");

        if (file && temp) {
            int curr_id, departmentId;
            char firstName[50], lastName[50], curr_email[100];
            while (fscanf(file, "%d %s %s %s %d\n", &curr_id, firstName, lastName,
                          curr_email, &departmentId) != EOF) {
                if (curr_id == id) {
                    fprintf(temp, "%d %s %s %s %d\n", id, firstName, lastName, email, departmentId);
                } else {
                    fprintf(temp, "%d %s %s %s %d\n", curr_id, firstName, lastName, curr_email, departmentId);
                }
            }
            fclose(file);
            fclose(temp);
            remove("data/Instructors.txt");
            rename("data/Instructors_temp.txt", "data/Instructors.txt");
        }
    } else {
        printf("Instructor not found.\n");
    }

    release_lock(5, EXCLUSIVE);
}


void addInstructorPhoneNumber(int instructorId, char phone[]) {

    Instructor *inst = searchInstructorById(instructorId);
    if (!inst) {
        printf("Error: Instructor not found.\n");
        return;
    }

    if (strlen(phone) == 0 || strlen(phone) > 14) {
        printf("Error: Phone number must be between 1 and 14 characters.\n");
        return;
    }

    for (int i = 0; i < strlen(phone); i++) {
        if (i == 0 && phone[i] == '+') continue;
        if (!isdigit(phone[i])) {
            printf("Error: Phone number can only contain digits and optional + prefix.\n");
            return;
        }
    }

    int count = 0;
    for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++) {
        if (instructorPhoneNumbers[i].instructorId == instructorId) {
            count++;
        }
    }

    if (count >= MAX_PHONE_NUMBERS) {
        printf("Error: Maximum number of phone numbers reached for this instructor.\n");
        return;
    }

    for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++) {
        if (instructorPhoneNumbers[i].id == 0) {
            instructorPhoneNumbers[i].id = nextPhoneNumberId++;
            instructorPhoneNumbers[i].instructorId = instructorId;
            strncpy(instructorPhoneNumbers[i].phone, phone, 15);
            storeInstructorPhoneNumber(&instructorPhoneNumbers[i]);
            printf("Phone number added successfully.\n");
            return;
        }
    }

    printf("Error: Phone number storage is full.\n");
}

// Remove phone number from an instructor
void removeInstructorPhoneNumber(int phoneNumberId) {

    for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++) {
        if (instructorPhoneNumbers[i].id == phoneNumberId) {
            instructorPhoneNumbers[i].id = 0;
            instructorPhoneNumbers[i].instructorId = 0;
            printf("Phone number removed successfully.\n");
            return;
        }
    }

    printf("Error: Phone number not found.\n");
}

// Show all phone numbers of an instructor
void showInstructorPhoneNumbers(int instructorId) {

    Instructor *inst = searchInstructorById(instructorId);
    if (!inst) {
        printf("Error: Instructor not found.\n");
        return;
    }

    printf("\nInstructor ID: %d - %s %s\n", inst->id, inst->firstName, inst->lastName);
    printf("Phone Numbers:\n");

    bool found = false;
    for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++) {
        if (instructorPhoneNumbers[i].instructorId == instructorId) {
            printf("ID: %d, Phone: %s\n", 
                   instructorPhoneNumbers[i].id, 
                   instructorPhoneNumbers[i].phone);
            found = true;
        }
    }

    if (!found) {
        printf("No phone numbers found.\n");
    }

}

void *printInstructor(void *arg) {
    acquire_lock(5, SHARED);

    InstructorThreadArg *threadArg = (InstructorThreadArg *)arg;
    Instructor *instructor = threadArg->instructor;

    printf("Instructor ID: %d Name: %s %s Email: %s Department ID: %d \n",
           instructor->id, instructor->firstName, instructor->lastName,
           instructor->email, instructor->departmentId);

    release_lock(5, SHARED);
    return NULL;
}

void showAllInstructors() {
    acquire_lock(5, SHARED);

    printf("\nThere are currently %d instructor(s) in the database.\n", instructorCounter);

    pthread_t threads[instructorCounter];
    InstructorThreadArg args[instructorCounter];
    int index = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (instructorHashTable[i] != NULL && instructorHashTable[i]->occupied) {
            args[index].instructor = instructorHashTable[i];
            pthread_create(&threads[index], NULL, printInstructor, &args[index]);
            index++;
        }
    }

    for (int i = 0; i < index; i++) {
        pthread_join(threads[i], NULL);
    }

    release_lock(5, SHARED);
}

void showInstructor(Instructor *inst)
{
    if (!inst)
    {
        printf("No instructor data to display.\n");
        return;
    }

    printf("\n*********************************************");
    printf("\nInstructor ID: %d\n", inst->id);
    printf("Name: %s %s\n", inst->firstName, inst->lastName);
    printf("Email: %s\n", inst->email);
    printf("Department ID: %d\n", inst->departmentId);
}

Instructor *searchInstructorById(int id) {

    int index = hash(id);
    while (instructorHashTable[index] != NULL) {
        if (instructorHashTable[index]->occupied && instructorHashTable[index]->id == id) {
            return instructorHashTable[index];
        }
        index = hash(index + 1);
    }

    return NULL;
}

// Search instructor by name
Instructor *searchInstructorByName(char firstName[], char lastName[]) {

    for (int i = 0; i < instructorMappingCount; i++) {
        if (strcmp(instructorNameIdMapping[i].firstName, firstName) == 0 &&
            strcmp(instructorNameIdMapping[i].lastName, lastName) == 0) {
            release_lock(5, SHARED);
            return searchInstructorById(instructorNameIdMapping[i].id);
        }
    }

    return NULL;
}

// Search instructor by email
Instructor *searchInstructorByEmail(char email[]) {

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (instructorHashTable[i] != NULL && instructorHashTable[i]->occupied &&
            strcmp(instructorHashTable[i]->email, email) == 0) {
            release_lock(5, SHARED);
            return instructorHashTable[i];
        }
    }

    return NULL;
}

void searchInstructorsByDepartment(int departmentId)
{
    printf("\nInstructors in Department %d:\n", departmentId);
    bool found = false;
    
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        if (instructorHashTable[i] != NULL && instructorHashTable[i]->occupied == 1 &&
            instructorHashTable[i]->departmentId == departmentId)
        {
            showInstructor(instructorHashTable[i]);
            found = true;
        }
    }
    
    if (!found)
    {
        printf("No instructors found in this department.\n");
    }
}

// Menu Operations

Instructor *selectInstructorMenu()
{
    int selection;
    int searchId;
    char searchFirstName[50], searchLastName[50];
    char searchEmail[100];
    
    do
    {
        printf("\nSelecting an instructor... \n");
        printf("1- Select instructor by ID\n");
        printf("2- Select instructor by name\n");
        printf("3- Select instructor by email\n");
        printf("0- Return to main menu\n");
        printf("Enter your choice: ");
        scanf("%d", &selection);
        getchar();

        switch (selection)
        {
        case 1:
            printf("Enter the instructor's ID: ");
            scanf("%d", &searchId);
            getchar();
            Instructor *foundById = searchInstructorById(searchId);
            if (foundById != NULL)
            {
                showInstructor(foundById);
                return foundById;
            }
            printf("\nThe instructor with ID %d was not found.\n", searchId);
            return NULL;
        case 2:
            printf("Enter the instructor's first name: ");
            scanf("%s", searchFirstName);
            printf("Enter the instructor's last name: ");
            scanf("%s", searchLastName);
            getchar();
            Instructor *foundByName = searchInstructorByName(searchFirstName, searchLastName);
            if (foundByName != NULL)
            {
                showInstructor(foundByName);
                return foundByName;
            }
            printf("\nInstructor %s %s was not found.\n", searchFirstName, searchLastName);
            return NULL;
        case 3:
            printf("Enter the instructor's email: ");
            scanf("%s", searchEmail);
            getchar();
            Instructor *foundByEmail = searchInstructorByEmail(searchEmail);
            if (foundByEmail != NULL)
            {
                showInstructor(foundByEmail);
                return foundByEmail;
            }
            printf("\nNo instructor found with email %s.\n", searchEmail);
            return NULL;
        case 0:
            printf("\nReturning to main menu...\n");
            return NULL;
        default:
            printf("\nInvalid option! Please try again.\n");
            return NULL;
        }
    } while (selection != 5);
}

void insertInstructorMenu()
{
    Instructor *instructor = malloc(sizeof(Instructor));
    printf("\nAdding an instructor... \n");
    printf("Please enter the instructor's ID: ");
    scanf("%d", &(instructor->id));
    getchar();
    
    printf("Please enter the instructor's first name: ");
    scanf("%s", instructor->firstName);
    
    printf("Please enter the instructor's last name: ");
    scanf("%s", instructor->lastName);
    
    printf("Please enter the instructor's email: ");
    scanf("%s", instructor->email);
    
    // Check for unique email
    if (searchInstructorByEmail(instructor->email) != NULL)
    {
        printf("Error: An instructor with this email already exists.\n");
        free(instructor);
        return;
    }
    
    printf("Please enter the instructor's department ID: ");
    scanf("%d", &(instructor->departmentId));
    getchar();
    
    insertInstructor(instructor, false);
    
    if (instructor->occupied)
    {
        char choice;
        do {
            printf("Would you like to add a phone number? (y/n): ");
            scanf("%c", &choice);
            getchar();
            
            if (tolower(choice) == 'y')
            {
                char phone[15];
                printf("Enter phone number: ");
                scanf("%s", phone);
                getchar();
                
                // Check for unique phone number
                bool isUnique = true;
                for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++)
                {
                    if (instructorPhoneNumbers[i].id != 0 && 
                        strcmp(instructorPhoneNumbers[i].phone, phone) == 0)
                    {
                        printf("Error: This phone number is already registered.\n");
                        isUnique = false;
                        break;
                    }
                }
                
                if (isUnique)
                {
                    addInstructorPhoneNumber(instructor->id, phone);
                }
            }
        } while (tolower(choice) == 'y');
    }
}

void manageInstructorPhoneNumbersMenu(int instructorId)
{
    int choice;
    char phone[15];
    int phoneNumberId;
    
    do
    {
        printf("\nManage Instructor Phone Numbers\n");
        printf("1- Add phone number\n");
        printf("2- Remove phone number\n");
        printf("3- Show phone numbers\n");
        printf("4- Return to main menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();
        
        switch(choice)
        {
        case 1:
            printf("Enter phone number: ");
            scanf("%s", phone);
            getchar();
            
            // Check for unique phone number
            bool isUnique = true;
            for (int i = 0; i < HASH_TABLE_SIZE * MAX_PHONE_NUMBERS; i++)
            {
                if (instructorPhoneNumbers[i].id != 0 && 
                    strcmp(instructorPhoneNumbers[i].phone, phone) == 0)
                {
                    printf("Error: This phone number is already registered.\n");
                    isUnique = false;
                    break;
                }
            }
            
            if (isUnique)
            {
                addInstructorPhoneNumber(instructorId, phone);
            }
            break;
        case 2:
            showInstructorPhoneNumbers(instructorId);
            printf("Enter phone number ID to remove: ");
            scanf("%d", &phoneNumberId);
            getchar();
            removeInstructorPhoneNumber(phoneNumberId);
            break;
        case 3:
            showInstructorPhoneNumbers(instructorId);
            break;
        case 4:
            printf("\nReturning to main menu...\n");
            break;
        default:
            printf("\nInvalid option! Please try again.\n");
        }
    } while (choice != 4);
}

void updateInstructorMenu()
{
    Instructor *inst = selectInstructorMenu();
    if (!inst) return;

    int choice;
    do
    {
        printf("\nUpdate Instructor\n");
        printf("1- Update email\n");
        printf("2- Manage phone numbers\n");
        printf("3- Return to main menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();
        
        switch(choice)
        {
        case 1:
            {
                char email[100];
                printf("Enter new email: ");
                scanf("%s", email);
                getchar();
                updateInstructor(inst->id, email);
            }
            break;
        case 2:
            manageInstructorPhoneNumbersMenu(inst->id);
            break;
        case 3:
            printf("\nReturning to main menu...\n");
            break;
        default:
            printf("\nInvalid option! Please try again.\n");
        }
    } while (choice != 3);
}

void deleteInstructorMenu()
{
    Instructor *inst = selectInstructorMenu();
    if (!inst) return;

    printf("Are you sure you want to delete this instructor? (y/n): ");
    char confirm;
    scanf("%c", &confirm);
    getchar();
    
    if (tolower(confirm) == 'y')
    {
        deleteInstructor(inst->id);
    }
    else
    {
        printf("Delete cancelled.\n");
    }
}

void viewInstructorPhoneNumbersMenu()
{
    Instructor *inst = selectInstructorMenu();
    if (inst)
    {
        showInstructorPhoneNumbers(inst->id);
    }
}

void instructorMenu()
{
    int choice;
    do
    {
        printf("\nInstructor Management\n");
        printf("1- Show all instructors\n");
        printf("2- Show instructor\n");
        printf("3- Add instructor\n");
        printf("4- Update instructor\n");
        printf("5- Delete instructor\n");
        printf("6- View instructor phone numbers\n");
        printf("0- Return to main menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar();
        
        switch(choice)
        {
        case 1:
            showAllInstructors();
            break;
        case 2:
            selectInstructorMenu();
            break;
        case 3:
            insertInstructorMenu();
            break;
        case 4:
            updateInstructorMenu();
            break;
        case 5:
            deleteInstructorMenu();
            break;
        case 6:
            viewInstructorPhoneNumbersMenu();
            break;
        case 0:
            printf("\nReturning to main menu...\n");
            break;
        default:
            printf("\nInvalid option! Please try again.\n");
        }
    } while (choice != 0);
}
