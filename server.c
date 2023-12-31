#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>


int globalThreadCounter[10];

pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;

void attachToSharedMemory(sharedMemory **sharedData);
void startTestMode(sharedMemory *sharedData);


uint32_t receiveFromClient(sharedMemory *sharedData);

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data);


uint32_t rotateRight(uint32_t k, unsigned int b);

void trialDivision(sharedMemory *sharedData, uint32_t n, int slot_index);

int findFreeSlot(sharedMemory *sharedData);


void *threadFunction(void *arg);

typedef struct {
    uint32_t input;
    sharedMemory *sharedData;
} ThreadArgs;


typedef struct {
    int input;
    sharedMemory *sharedData;
} TestThreadArgs;


int main() {





    sharedMemory *sharedData;
    attachToSharedMemory(&sharedData);





    pthread_t threads[320];

    ThreadArgs threadArgs[320];





    while (1) {

        uint32_t data = receiveFromClient(sharedData);

        if (data == 0) {  // If in test mode
            startTestMode(sharedData);
            continue;  // Go back to the start of the loop to wait for new requests
        }


        int start = 32 * sharedData->number;
        int end = start + 32;

        for (int i = start; i < end; i++) {
            threadArgs[i].sharedData = sharedData;
            uint32_t rotatedInput = rotateRight(data, i);
            printf("\n%u\n ", rotatedInput);
            threadArgs[i].input = rotatedInput;

            if (pthread_create(&threads[i], NULL, threadFunction, &threadArgs[i]) != 0) {
                perror("pthread_create");
                return 1;
            }

            usleep(10000);
        }

    }



    // Wait for threads to finish using pthread_join
    for (int i = 0; i < 32; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }



//    printf("Received data from client: %u\n", trialDivision(data));
//    sendTOClient(sharedData,0, trialDivision(data));






    return 0;
}


void attachToSharedMemory(sharedMemory **sharedData) {
    int shmID = shmget(SHM_KEY, sizeof(sharedMemory), 0666);
    if (shmID == -1) {
        perror("Failed to find shared memory segment. Is the server running?");
        exit(1);
    }
    *sharedData = (sharedMemory *) shmat(shmID, NULL, 0);
    if (*sharedData == (void *)-1) {
        perror("Attachment failed");
        exit(1);
    }
}
uint32_t receiveFromClient(sharedMemory *sharedData) {
    while (sharedData->clientFlag == 0);
    uint32_t data = sharedData->number;
    int slot = findFreeSlot(sharedData);
    sharedData->number = slot;
    sharedData->clientFlag = 0;
    return data;
}

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data) {
    while (sharedData->serverFlag[slot_index] == 1);
    sharedData->slot[slot_index] = data;
    sharedData->serverFlag[slot_index] = 1;
}



uint32_t rotateRight(uint32_t k, unsigned int b) {
    return (k >> b) | (k << (32 - b));
}


void trialDivision(sharedMemory *sharedData, uint32_t n, int slot_index) {
    // Print the number of 2s that divide n
    while (n % 2 == 0) {
        wait_semaphore(&(sharedData->sem));
        sharedData->slot[slot_index] = 2;
        sendTOClient(sharedData, slot_index, sharedData->slot[slot_index]);
        signal_semaphore(&(sharedData->sem));

        n /= 2;
    }

    // n must be odd at this point, so skip even numbers and iterate only for odd integers
    for (uint32_t f = 3; f * f <= n; f += 2) {
        // While f divides n, print it and divide n
        while (n % f == 0) {
            wait_semaphore(&(sharedData->sem));
            sharedData->slot[slot_index] = f;
            sendTOClient(sharedData, slot_index, sharedData->slot[slot_index]);
            signal_semaphore(&(sharedData->sem));
            n /= f;
        }
    }

    // If n is a prime number greater than 2
    if (n > 2) {
        wait_semaphore(&(sharedData->sem));
        sharedData->slot[slot_index] = n;
        sendTOClient(sharedData, slot_index, sharedData->slot[slot_index]);
        signal_semaphore(&(sharedData->sem));
    }
}


void *threadFunction(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    uint32_t input = args->input;

    // Here's the change
    sharedMemory *sharedDataPointer = args->sharedData;

    int slot_index = sharedDataPointer->number;

    trialDivision(sharedDataPointer, input, slot_index);

    pthread_mutex_lock(&counterMutex);
    globalThreadCounter[slot_index]++;
    pthread_mutex_unlock(&counterMutex);

    if ((globalThreadCounter[slot_index]*100)/32 - sharedDataPointer->progress[slot_index] >= 5){
        sharedDataPointer->progress[slot_index] = (globalThreadCounter[slot_index]*100)/32;
    }


    return NULL;
}

int findFreeSlot(sharedMemory *sharedData) {
    for (int i = 0; i < 10; i++) {
        if (sharedData->progress[i] == 0) {
            globalThreadCounter[i] = 0;
            return i;
        }
    }
    return -1;  // No free slot found
}


void* testThreadFunction(void* arg) {
    TestThreadArgs *args = (TestThreadArgs *)arg;
    int threadNum = args->input;

    sharedMemory *sharedData = args->sharedData;

    wait_semaphore(&(sharedData->sem));


    sendTOClient(sharedData, threadNum / 10, threadNum);

    signal_semaphore(&(sharedData->sem));

    usleep((rand() % 91 + 10) * 1000);  // Random sleep between 10ms and 100ms


    return NULL;
}



void startTestMode(sharedMemory *sharedData) {
    pthread_t testThreads[30];
    TestThreadArgs threadArgs[30];

    for (int i = 0; i < 30; i++) {
        threadArgs[i].input = i;
        threadArgs[i].sharedData = sharedData;
        if (pthread_create(&testThreads[i], NULL, testThreadFunction, &threadArgs[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < 30; i++) {
        if (pthread_join(testThreads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }

    printf("Test mode completed!\n");
}