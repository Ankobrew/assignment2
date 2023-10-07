#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


void attachToSharedMemory(sharedMemory **sharedData);



void *sendToServer(void *arg);


void *readFromServer(void *args);

uint32_t getInput();

typedef struct {
    uint32_t input;
    sharedMemory *sharedData;
} ThreadArgs;

int main() {

    sharedMemory *sharedData;

    pthread_t sendThread, readThread;
    attachToSharedMemory(&sharedData);
    ThreadArgs args;
    args.sharedData = sharedData;

    pthread_create(&readThread, NULL, readFromServer, sharedData);

    while (1) { // Keep the main thread running to accept user inputs
        args.input = getInput();
        pthread_create(&sendThread, NULL, sendToServer, &args);
        pthread_detach(sendThread); // So we don't need to join later
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

    while (1) { // Keep running indefinitely
        for (int i = 0; i < 10; i++) { // Assuming 10 slots
            if (sharedData->serverFlag[i] == 1) {
                printf("Data from server (slot %d): %u\n", i, sharedData->slot[0]);
                printf("Progress: Query 1: %d%%\n", sharedData->progress[i]);
                sharedData->serverFlag[i] = 0;
            }
        }
        usleep(10000);
//        printf("Progress: Query 1: %d%%\n", (sharedData->progress[0])
    }

    return NULL;
}



uint32_t getInput() {
    printf("Enter a 32-bit integer (or 'q' to quit): ");
    char input[20];
    fgets(input, sizeof(input), stdin);
    if (input[0] == 'q' || input[0] == 'Q') {
        exit(0);
    }
    return (uint32_t) strtoul(input, NULL, 10);
}