#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_KEY 0x1234


typedef struct {
    uint32_t number;
    int slot[10];
    char clientFlag;
    char serverFlag[10];
} sharedMemory;

#endif
