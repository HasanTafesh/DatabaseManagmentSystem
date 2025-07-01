# University Database Management System (DBMS)

## Project Overview

This is a comprehensive University Database Management System implemented in C, designed to manage academic data including departments, instructors, students, courses, and enrollments. The system features a robust architecture with hash table-based data storage, concurrency control mechanisms, and a user-friendly command-line interface.

**Author:** Hasan Tafesh
**Course:** Database Programming  
**Semester:** Fall 2024

## Features

### 🏗️ **Core Functionality**
- **Complete CRUD Operations**: Create, Read, Update, Delete for all entities
- **Hash Table Storage**: Efficient data retrieval using hash-based indexing
- **Concurrency Control**: Multi-threading support with lock management
- **Data Persistence**: File-based storage with automatic data loading/saving
- **Input Validation**: Comprehensive data validation and error handling
- **Search Capabilities**: Multiple search methods (ID, name, email, phone)

### 📊 **Entity Management**
- **Departments**: Academic department management
- **Instructors**: Faculty member management with multiple phone numbers
- **Students**: Student records with department associations
- **Courses**: Course catalog with instructor and department assignments
- **Enrollments**: Student-course relationships with grades and status tracking

### 🔒 **Advanced Features**
- **Lock Management**: Thread-safe operations with shared/exclusive locks
- **Dynamic Resizing**: Automatic hash table expansion for optimal performance
- **Foreign Key Validation**: Referential integrity across entities
- **Statistics & Reports**: Enrollment counts, course statistics, and relationship queries

## System Architecture

### Data Structures

#### Department
```c
typedef struct Department {
    int id;                     // Primary key
    char name[100];             // Department name
    char phone[15];             // Contact phone
    int occupied;               // Hash table slot flag
} Department;
```

#### Student
```c
typedef struct Student {
    int id;                     // Primary key
    char firstName[50];         // First name
    char lastName[50];          // Last name
    char email[100];           // Email address
    char phone[15];            // Phone number
    int departmentId;          // Foreign key to Department
    int occupied;              // Hash table slot flag
} Student;
```

#### Course
```c
typedef struct Course {
    int id;                     // Primary key
    char title[100];           // Course title
    int credits;               // Credit hours
    int departmentId;          // Foreign key to Department
    int instructorId;          // Foreign key to Instructor
    int occupied;              // Hash table slot flag
} Course;
```

#### Instructor
```c
typedef struct Instructor {
    int id;                     // Primary key
    char firstName[50];         // First name
    char lastName[50];          // Last name
    char email[100];           // Email address
    int departmentId;          // Foreign key to Department
    int occupied;              // Hash table slot flag
} Instructor;
```

#### Enrollment
```c
typedef struct Enrollment {
    int id;                    // Primary key
    int studentId;             // Foreign key to Student
    int courseId;              // Foreign key to Course
    char grade[3];             // Student grade (A, B+, etc.)
    EnrollmentStatus status;   // ENROLLED, DROPPED, COMPLETED
    int occupied;              // Hash table slot flag
} Enrollment;
```

### Concurrency Control

The system implements a sophisticated lock management system:

```c
typedef enum { NONE, SHARED, EXCLUSIVE } LockType;

typedef struct {
    int table_id;
    LockType lock_type;
    int lock_count;
    pthread_mutex_t lock_mutex;
    pthread_cond_t lock_cond;
} Lock;
```

- **Shared Locks**: Multiple readers can access data simultaneously
- **Exclusive Locks**: Single writer access with mutual exclusion
- **Condition Variables**: Efficient thread synchronization

## File Structure

```
DBP_FinalCode_HasanTafesh_24110630/
├── include/                     # Header files
│   ├── common.h                # Common utilities and hash functions
│   ├── department.h            # Department data structures and operations
│   ├── instructor.h            # Instructor data structures and operations
│   ├── student.h               # Student data structures and operations
│   ├── course.h                # Course data structures and operations
│   ├── enrollment.h            # Enrollment data structures and operations
│   └── lock_management.h       # Concurrency control mechanisms
├── src/                        # Source code implementation
│   ├── main.c                  # Main application entry point
│   ├── common.c                # Common utility implementations
│   ├── department.c            # Department CRUD operations
│   ├── instructor.c            # Instructor CRUD operations
│   ├── student.c               # Student CRUD operations
│   ├── course.c                # Course CRUD operations
│   ├── enrollment.c            # Enrollment CRUD operations
│   └── lock_management.c       # Lock management implementation
├── data/                       # Data storage files
│   ├── Departments.txt         # Department records
│   ├── Instructors.txt         # Instructor records
│   ├── instructor_phones.txt   # Instructor phone numbers
│   ├── Students.txt            # Student records
│   ├── Courses.txt             # Course records
│   └── Enrollments.txt         # Enrollment records
├── universiy_dbms_final.exe    # Compiled executable
└── README.md                   # This documentation
```

## Installation & Compilation

### Prerequisites
- GCC compiler
- POSIX threads library (pthread)
- Windows: MinGW or similar C compiler
- Linux/macOS: Standard development tools

### Compilation

#### Method 1: Compile from source
```bash
gcc -I include src/*.c -o university_dbms_final
```

#### Method 2: Run pre-compiled executable
```bash
./university_dbms_final.exe
```

### Data Files Format

#### Departments.txt
```
ID Name Phone
1 IT 5223352532
2 Eng 23455432121
```

#### Students.txt
```
ID FirstName LastName Email Phone DepartmentID
1 hasan tafesh hasan@gmail.com 112211221 1
```

#### Courses.txt
```
ID Title Credits DepartmentID InstructorID
1 IT 3 1 1
```

## Usage Guide

### Main Menu
The system presents a hierarchical menu structure:

```
=== University Database Management System ===
1. Department Operations
2. Instructor Operations
3. Student Operations
4. Course Operations
5. Enrollment Operations
0. Exit
```

### Department Operations
- **Insert Department**: Add new academic departments
- **Update Department**: Modify department contact information
- **Delete Department**: Remove departments (with validation)
- **Search Departments**: Find by ID, name, or phone
- **View All Departments**: Display complete department list
- **Department Reports**: View instructors, courses, and students in departments

### Student Operations
- **Insert Student**: Add new student records
- **Update Student**: Modify student information
- **Delete Student**: Remove student records
- **Search Students**: Find by ID, name, email, or phone
- **View All Students**: Display complete student list
- **Student Reports**: View enrolled courses and grades

### Course Operations
- **Insert Course**: Add new courses to catalog
- **Update Course**: Modify course assignments
- **Delete Course**: Remove courses from catalog
- **Search Courses**: Find by ID or title
- **View All Courses**: Display complete course list
- **Course Reports**: View enrolled students and statistics

### Instructor Operations
- **Insert Instructor**: Add new faculty members
- **Update Instructor**: Modify instructor information
- **Delete Instructor**: Remove instructor records
- **Manage Phone Numbers**: Add/remove instructor contact numbers
- **Search Instructors**: Find by ID, name, or email
- **Instructor Reports**: View assigned courses

### Enrollment Operations
- **Insert Enrollment**: Register students for courses
- **Update Grade**: Modify student grades
- **Update Status**: Change enrollment status (Enrolled/Dropped/Completed)
- **Delete Enrollment**: Remove course registrations
- **Search Enrollments**: Find by student or course
- **Enrollment Reports**: View course statistics and student records

## Technical Implementation Details

### Hash Table Implementation
- **Initial Size**: 100 slots
- **Load Factor**: 75% threshold for resizing
- **Collision Resolution**: Linear probing
- **Dynamic Resizing**: Automatic table expansion

### Data Validation
- **Input Sanitization**: Length and format validation
- **Referential Integrity**: Foreign key validation
- **Duplicate Prevention**: Unique constraint enforcement
- **Phone Number Validation**: Format and character validation

### Performance Optimizations
- **Binary Search**: Fast ID-based lookups
- **Name Mapping**: Efficient name-based searches
- **Memory Management**: Dynamic allocation with proper cleanup
- **File I/O Optimization**: Buffered writes and atomic operations

### Thread Safety
- **Lock Granularity**: Table-level locking
- **Deadlock Prevention**: Consistent lock ordering
- **Concurrent Access**: Multiple readers, single writer model
- **Condition Variables**: Efficient thread synchronization

## Error Handling

The system implements comprehensive error handling:

- **Memory Allocation Errors**: Graceful failure with user notification
- **File I/O Errors**: Robust file operation handling
- **Input Validation**: User-friendly error messages
- **Concurrency Errors**: Thread-safe error recovery
- **Data Integrity**: Referential integrity enforcement

## Sample Data

The system comes with sample data files:

### Departments
- IT Department (ID: 1)
- Engineering Department (ID: 2)
- Medical Department (ID: 3)
- Arts Department (ID: 4)

### Students
- Hasan Tafesh (ID: 1) - IT Department
- Rasha Mahmoud (ID: 2) - IT Department

### Courses
- IT Course (ID: 1) - 3 credits
- Operating Systems (ID: 2) - 3 credits
- Computer Programming (ID: 3) - 6 credits

## Future Enhancements

Potential improvements for the system:

1. **Database Integration**: SQLite or MySQL backend
2. **Web Interface**: RESTful API with web frontend
3. **Advanced Queries**: Complex reporting and analytics
4. **User Authentication**: Role-based access control
5. **Data Export**: CSV, JSON, and XML export capabilities
6. **Backup & Recovery**: Automated data backup systems
7. **Performance Monitoring**: Query optimization and metrics
8. **Mobile Support**: Cross-platform mobile application

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   - Ensure GCC is installed and in PATH
   - Check pthread library availability
   - Verify all source files are present

2. **Runtime Errors**
   - Check data file permissions
   - Ensure data directory exists
   - Verify file format compliance

3. **Performance Issues**
   - Monitor hash table load factor
   - Check for memory leaks
   - Optimize file I/O operations

### Debug Mode
The system includes commented concurrency test code in `main.c` for debugging multi-threaded operations.

## License

This project is developed as part of the Database Programming course at the university. All rights reserved.

## Contact

**Developer:** Hasan Tafesh  
**Student ID:** 24110630  
**Course:** Database Programming  
**Institution:** University Database Programming Course

---

*This README provides comprehensive documentation for the University Database Management System. For technical support or questions, please refer to the course instructor or review the source code comments.* 