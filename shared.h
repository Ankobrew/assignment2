#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define SHM_KEY 0x1234

typedef struct {
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Semaphore;


typedef struct {
    uint32_t number;
    int slot[10];
    char clientFlag;
    char serverFlag[10];
    Semaphore sem;
} sharedMemory;




void init_semaphore(Semaphore *sem, int value);
void wait_semaphore(Semaphore *sem);
void signal_semaphore(Semaphore *sem);
void destroy_semaphore(Semaphore *sem);

#endif
