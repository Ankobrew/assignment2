#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void attachToSharedMemory(sharedMemory **sharedData);

int initializeSharedMemory(sharedMemory **sharedData);
void cleanupSharedMemory(sharedMemory *sharedData, int shmID);

void *sendToServer(void *arg);


void *readFromServer(void *args);

uint32_t getInput(sharedMemory *sharedData, int shmID);

typedef struct {
    uint32_t input;
    sharedMemory *sharedData;
} ThreadArgs;

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

    while(1) {  // Infinite loop to continuously check if it can send more requests
        if (activeRequests < 10) {  // Check active request count
            args.input = getInput(sharedData, shmID);
            pthread_create(&sendThread, NULL, sendToServer, &args);
            pthread_detach(sendThread);  // So we don't need to join later
            activeRequests++;  // Increment active request count
        } else {
            printf("System is busy. Please wait and try again.\n");
            for (int i = 0; i < 10; i++) {
                if (sharedData->progress[i] == 100) {
                    sharedData->progress[i] = 0;
                    printf("%d", sharedData->progress[i]);
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

void *keepCheckingForFreeThread(void *arg){
    sharedMemory *sharedData = (sharedMemory *)arg;

    while (1){
        for (int i = 0; i < 10 ; ++i) {

        }
    }
}


void *readFromServer(void *args) {
    sharedMemory *sharedData = (sharedMemory *)args;

    while (1) { // Keep running indefinitely
        for (int i = 0; i < 10; i++) { // Assuming 10 slots
            if (sharedData->serverFlag[i] == 1) {
                printf("Data from server (slot %d): %u\n", i, sharedData->slot[i]);
                printf("Progress: Query %d: %d%%\n", i, sharedData->progress[i]);
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
    char input[20];
    fgets(input, sizeof(input), stdin);
    if (input[0] == 'q' || input[0] == 'Q') {
        cleanupSharedMemory(sharedData, shmID);
        exit(0);
    }
    return (uint32_t) strtoul(input, NULL, 10);
}