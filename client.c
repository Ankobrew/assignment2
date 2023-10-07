#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

void attachToSharedMemory(sharedMemory **sharedData);

int initializeSharedMemory(sharedMemory **sharedData);
void cleanupSharedMemory(sharedMemory *sharedData, int shmID);

void *sendToServer(void *arg);

void displayProgress(sharedMemory *sharedData);


void *readFromServer(void *args);

uint32_t getInput(sharedMemory *sharedData, int shmID);

int Mode = 0;

typedef struct {
    int input;
    sharedMemory *sharedData;
} ThreadArgs;


typedef struct {
    struct timeval requestTime;
    int isActive;
} RequestTimeInfo;

RequestTimeInfo requestTimes[10] = {0};

int main() {

    sharedMemory *sharedData = (sharedMemory *)malloc(sizeof(sharedMemory));
    if (sharedData == NULL) {
        perror("Failed to allocate memory for sharedData");
        exit(1);
    }

    int activeRequests = 0;

    int shmID = initializeSharedMemory(&sharedData);
    //cleanupSharedMemory(sharedData, shmID);


    pthread_t sendThread, readThread;
    ThreadArgs args;
    args.sharedData = sharedData;

    pthread_create(&readThread, NULL, readFromServer, sharedData);

    while(1) {
        // Infinite loop to continuously check if it can send more requests
        if (activeRequests < 10) {  // Check active request count
            args.input = getInput(sharedData, shmID);
            pthread_create(&sendThread, NULL, sendToServer, &args);
            pthread_detach(sendThread);  // So we don't need to join later
            if (args.input == 0 && activeRequests == 0) {  // If in test mode
                while (sharedData->progress[0]!= 100);
                printf("Test Mode Complete\n");
                sharedData->progress[0] = 0;
                Mode = 2;
                continue;
            }
            else if (args.input == 0 && activeRequests != 0)
            {
                printf("Warning");
            } else {
                Mode = 1;
                activeRequests++;
            }// Increment active request count
        } else {

            printf("System is busy. Please wait and try again.\n");

        }


        for (int i = 0; i < 10; i++) {
            if (sharedData->progress[i] == 0) {
                if (activeRequests == 0){
                    break;
                } else {
                    activeRequests--;
                }
            }
        }

        usleep(50000);  // Slight delay to avoid busy-waiting too aggressively
    }  // Slight delay to avoid busy-waiting too aggressively

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




void *sendToServer(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    uint32_t data = args->input;
    sharedMemory *sharedData = args->sharedData;
    while (sharedData->clientFlag == 1);
    sharedData->number = data;
    sharedData->clientFlag = 1;

    return NULL;
}




void *readFromServer(void *args) {
    sharedMemory *sharedData = (sharedMemory *)args;

    struct timeval lastProgressUpdateTime;
    gettimeofday(&lastProgressUpdateTime, NULL);

    while (1) { // Keep running indefinitely

        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long elapsed = (currentTime.tv_sec - lastProgressUpdateTime.tv_sec) * 1000 +
                       (currentTime.tv_usec - lastProgressUpdateTime.tv_usec) / 1000;

        // If it has been more than 500ms or a response was received, update the progress
        if (elapsed >= 500 && Mode == 1) {  // Do not display progress in test mode
            displayProgress(sharedData);
            gettimeofday(&lastProgressUpdateTime, NULL); // Reset the timer
        }

        for (int i = 0; i < 10; i++) { // Assuming 10 slots
            if (sharedData->serverFlag[i] == 1) {
                printf("Data from server (slot %d): %u\n", i, sharedData->slot[i]);
                sharedData->serverFlag[i] = 0;
            }
        }
        usleep(10000);
//
    }

    return NULL;
}



uint32_t getInput(sharedMemory *sharedData, int shmID) {
    printf("Enter a 32-bit integer (or 'q' to quit): ");
    char input[32];
    fgets(input, sizeof(input), stdin);
    if (input[0] == 'q' || input[0] == 'Q') {
        cleanupSharedMemory(sharedData, shmID);
        exit(0);
    }
    if (input[0] == '0') {
        printf("Entering test mode...\n");
        Mode = 2;
        return 0;  // Return 0 to signal test mode
    }
    return (uint32_t) strtoul(input, NULL, 10);
}

void displayProgress(sharedMemory *sharedData) {
    printf("Current Progress: ");
    for (int i = 0; i < 10; i++) {
        if (sharedData->serverFlag[i] == 1 || sharedData->progress[i] > 0) {
            printf("Q%d:%d%% ", i+1, sharedData->progress[i]);
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