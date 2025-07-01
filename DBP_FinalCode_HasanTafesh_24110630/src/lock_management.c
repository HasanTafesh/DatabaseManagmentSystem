// lock_management.c
#include "lock_management.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_TABLES 5

typedef struct {
    int table_id;
    LockType lock_type;
    int lock_count; // this is count of shared locks if lock_type == SHARED
    pthread_mutex_t lock_mutex;
    pthread_cond_t lock_cond;
} Lock;

Lock lock_table[MAX_TABLES];

void initialize_lock_table() {
    for (int i = 0; i < MAX_TABLES; i++) {
        lock_table[i].table_id = i + 1;
        lock_table[i].lock_type = NONE;
        lock_table[i].lock_count = 0;
        pthread_mutex_init(&lock_table[i].lock_mutex, NULL);
        pthread_cond_init(&lock_table[i].lock_cond, NULL);
    }
}

void acquire_lock(int table_id, LockType requested_lock) {
    Lock *lock = &lock_table[table_id - 1];

    pthread_mutex_lock(&lock->lock_mutex);

    while (1) {
        if (requested_lock == SHARED) {
            if (lock->lock_type == NONE || lock->lock_type == SHARED) {
                lock->lock_type = SHARED;
                lock->lock_count++;
                break;
            }
        } else if (requested_lock == EXCLUSIVE) {
            if (lock->lock_type == NONE) {
                lock->lock_type = EXCLUSIVE;
                break;
            }
        }
        pthread_cond_wait(&lock->lock_cond, &lock->lock_mutex);
    }

    pthread_mutex_unlock(&lock->lock_mutex);
}

void release_lock(int table_id, LockType lock_type) {
    Lock *lock = &lock_table[table_id - 1];

    pthread_mutex_lock(&lock->lock_mutex);
    if (lock_type == SHARED) {
        lock->lock_count--;
        if (lock->lock_count == 0) {
            lock->lock_type = NONE;
        }
    } else if (lock_type == EXCLUSIVE) {
        lock->lock_type = NONE;
    }

    pthread_cond_broadcast(&lock->lock_cond);
    pthread_mutex_unlock(&lock->lock_mutex);
}