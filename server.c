#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


int globalThreadCounter[10];

pthread_mutex_t counterMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t slotMutex = PTHREAD_MUTEX_INITIALIZER;


void attachToSharedMemory(sharedMemory **sharedData);


uint32_t receiveFromClient(sharedMemory *sharedData);

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data);


uint32_t rotateRight(uint32_t k, unsigned int b);

void trialDivision(sharedMemory *sharedData, uint32_t n, int slot_index);

int findFreeSlot(sharedMemory *sharedData);

void startTestMode(sharedMemory *sharedData);

void displayServerProgress(sharedMemory *sharedData);


void *threadFunction(void *arg);

typedef struct {
    uint32_t input;
    sharedMemory *sharedData;
    int slot_index;
} ThreadArgs;

typedef struct {
    int input;
    sharedMemory *sharedData;
} TestThreadArgs;


int main() {





    sharedMemory *sharedData;
    attachToSharedMemory(&sharedData);


    srand(time(NULL));


    pthread_t threads[320];

    ThreadArgs threadArgs[320];


    while (1) {

        uint32_t data = receiveFromClient(sharedData);

        if (data == 0) {  // If in test mode
            startTestMode(sharedData);
            sharedData->progress[0] = 100;
            continue;  // Go back to the start of the loop to wait for new requests
        }


        int start = 32 * sharedData->number;
        int end = start + 32;

        for (int i = start; i < end; i++) {
            threadArgs[i].sharedData = sharedData;
            uint32_t rotatedInput = rotateRight(data, i);
            threadArgs[i].input = rotatedInput;
            threadArgs[i].slot_index = sharedData->number;
            if (pthread_create(&threads[i], NULL, threadFunction, &threadArgs[i]) != 0) {
                perror("pthread_create");
                return 1;
            }

            usleep(10000);
        }

        displayServerProgress(sharedData);

    }



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
    pthread_mutex_lock(&slotMutex);
    int slot_index = findFreeSlot(sharedData);
    pthread_mutex_unlock(&slotMutex);
    sharedData->number = slot_index;
    gettimeofday(&sharedData->startTime[slot_index], NULL);
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
    sharedMemory *sharedData = args->sharedData;

    int slot_index = args->slot_index;

    trialDivision(sharedData, input, slot_index);

    pthread_mutex_lock(&counterMutex);
    globalThreadCounter[slot_index]++;

    if (globalThreadCounter[slot_index] == 32) {
        gettimeofday(&sharedData->endTime[slot_index], NULL);

        sharedData->timeElapsed[slot_index] = (sharedData->endTime[slot_index].tv_sec - sharedData->startTime[slot_index].tv_sec) * 1000 +
                       (sharedData->endTime[slot_index].tv_usec - sharedData->startTime[slot_index].tv_usec) / 1000;
        sharedData->progress[slot_index] = 100;
        globalThreadCounter[slot_index] = 0;  // Reset the thread counter for this slot
        sharedData->progress[slot_index] = 0; // Mark as completed
    }

    else if ((globalThreadCounter[slot_index]*100)/32 - sharedData->progress[slot_index] >= 5){
        sharedData->progress[slot_index] = (globalThreadCounter[slot_index]*100)/32;
    }

    pthread_mutex_unlock(&counterMutex);


    return NULL;
}

int findFreeSlot(sharedMemory *sharedData) {
    for (int i = 0; i < 10; i++) {
        if (sharedData->progress[i] == 0) {
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
        }
    }


    printf("Test mode completed!\n");
}

void displayServerProgress(sharedMemory *sharedData) {
    printf("\nServer Progress: ");
    for (int i = 0; i < 10; i++) {
        if (sharedData->progress[i] > 0 || sharedData->serverFlag[i] == 1) {
            printf("Slot %d: %d%% ", i, sharedData->progress[i]);
            int bars = sharedData->progress[i] / 10; // Assuming 10 blocks for 100%
            for (int j = 0; j < bars; j++) {
                printf("▓");
            }
            for (int j = bars; j < 10; j++) {
                printf("_");
            }
            printf("| ");
        }
    }
    printf("\n");
}