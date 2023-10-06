#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>




int initializeSharedMemory(sharedMemory **sharedData);

uint32_t receiveFromClient(sharedMemory *sharedData);

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data);

void cleanupSharedMemory(sharedMemory *sharedData, int shmID);

uint32_t rotateRight(uint32_t k, unsigned int b);

void trialDivision(sharedMemory *sharedData, uint32_t n);


void *threadFunction(void *arg);

typedef struct {
    uint32_t input;
    sharedMemory *sharedData;
} ThreadArgs;


int main() {

    int shmID;
    sharedMemory *sharedData = (sharedMemory *)malloc(sizeof(sharedMemory));
    if (sharedData == NULL) {
        perror("Failed to allocate memory for sharedData");
        exit(1);
    }




    shmID = initializeSharedMemory(&sharedData);

    uint32_t data = receiveFromClient(sharedData);




    pthread_t threads[32];

    ThreadArgs threadArgs;

    threadArgs.sharedData = sharedData;
//
//
//
    for (int i = 0; i < 32; i++) {
        uint32_t rotatedInput = rotateRight(data, i);
        printf("\n%u\n ", rotatedInput);
        threadArgs.input= rotatedInput;

        if (pthread_create(&threads[i], NULL, threadFunction, &threadArgs) != 0) {
            perror("pthread_create");
            return 1;
        }

        usleep(10000);
    }

    // Wait for threads to finish using pthread_join
    for (int i = 0; i < 5; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }



//    printf("Received data from client: %u\n", trialDivision(data));
//    sendTOClient(sharedData,0, trialDivision(data));

    cleanupSharedMemory(sharedData, shmID);




    return 0;
}


int initializeSharedMemory(sharedMemory **sharedData) {
    int shmID = shmget(SHM_KEY, sizeof(sharedMemory), 0666 | IPC_CREAT);
    if (shmID == -1) {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    *sharedData = (sharedMemory *)shmat(shmID, NULL, 0);
    if (*sharedData == (void *)-1) {
        perror("Attachment failed");
        exit(1);
    }

    init_semaphore(&((*sharedData)->sem), 1);


    return shmID;
}

uint32_t receiveFromClient(sharedMemory *sharedData) {
    while (sharedData->clientFlag == 0);
    uint32_t data = sharedData->number;
    sharedData->number = 0;
    sharedData->clientFlag = 0;
    return data;
}

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data) {
    while (sharedData->serverFlag[slot_index] == 1);
    sharedData->slot[slot_index] = data;
    sharedData->serverFlag[slot_index] = 1;
}


void cleanupSharedMemory(sharedMemory *sharedData, int shmID) {

    destroy_semaphore(&(sharedData->sem));
    if (shmdt(sharedData) == -1) {
        perror("Detached from shared memory failed");
        exit(1);
    }

    // Remove the shared memory segment
    if (shmctl(shmID, IPC_RMID, NULL) == -1) {
        perror("Remove shared Memory failed");
        exit(1);
    }
}

uint32_t rotateRight(uint32_t k, unsigned int b) {
    return (k >> b) | (k << (32 - b));
}


void trialDivision(sharedMemory *sharedData, uint32_t n) {
    // Print the number of 2s that divide n
    while (n % 2 == 0) {

        wait_semaphore(&(sharedData->sem));
        sharedData->slot[0] = 2;
        printf("%lu ", sharedData->slot[0]);
        signal_semaphore(&(sharedData->sem));

        n /= 2;
    }

    // n must be odd at this point, so skip even numbers and iterate only for odd integers
    for (uint32_t f = 3; f * f <= n; f += 2) {
        // While f divides n, print it and divide n
        while (n % f == 0) {

            wait_semaphore(&(sharedData->sem));
            sharedData->slot[0] = f;
            printf("%lu ", sharedData->slot[0]);
            signal_semaphore(&(sharedData->sem));
            n /= f;
        }
    }

    // If n is a prime number greater than 2
    if (n > 2) {
        wait_semaphore(&(sharedData->sem));
        sharedData->slot[0] = n;
        printf("%lu ", sharedData->slot[0]);
        signal_semaphore(&(sharedData->sem));
    }
}


void *threadFunction(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    uint32_t input = args->input;

    // Here's the change
    sharedMemory *sharedDataPointer = args->sharedData;

    trialDivision(sharedDataPointer, input);


    return NULL;
}