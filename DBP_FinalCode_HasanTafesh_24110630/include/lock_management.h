// lock_management.h
#ifndef LOCK_MANAGEMENT_H
#define LOCK_MANAGEMENT_H

#include <pthread.h>

typedef enum { NONE, SHARED, EXCLUSIVE } LockType;

void initialize_lock_table();
void acquire_lock(int table_id, LockType lock_type);
void release_lock(int table_id, LockType lock_type);

#endif
