#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/time.h>

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
    int progress[10];
    Semaphore sem;
    struct timeval startTime[10];
    struct timeval endTime[10];
    long timeElapsed[10];
} sharedMemory;




void init_semaphore(Semaphore *sem, int value);
void wait_semaphore(Semaphore *sem);
void signal_semaphore(Semaphore *sem);
void destroy_semaphore(Semaphore *sem);

#endif
