#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>

void attachToSharedMemory(sharedMemory **sharedData);

int initializeSharedMemory(sharedMemory **sharedData);
void cleanupSharedMemory(sharedMemory *sharedData, int shmID);

void *sendToServer(void *arg);

void displayProgress(sharedMemory *sharedData);


void *readFromServer(void *args);

int getInput(sharedMemory *sharedData, int shmID);

int Mode = 0;

typedef struct {
    int input;
    sharedMemory *sharedData;
} ThreadArgs;


int activeRequests = 0;

int main() {

    sharedMemory *sharedData = (sharedMemory *)malloc(sizeof(sharedMemory));
    if (sharedData == NULL) {
        perror("Failed to allocate memory for sharedData");
        exit(1);
    }



    int shmID = initializeSharedMemory(&sharedData);


    pthread_t sendThread, readThread;
    ThreadArgs args;
    args.sharedData = sharedData;

    pthread_create(&readThread, NULL, readFromServer, sharedData);

    while(1) {
        // Infinite loop to continuously check if it can send more requests
        if (activeRequests < 10) {  // Check active request count
            args.input = getInput(sharedData, shmID);
            if (args.input == 0 && activeRequests == 0) {
                pthread_create(&sendThread, NULL, sendToServer, &args);
                pthread_detach(sendThread);  // So we don't need to join later// If in test mode
                while (sharedData->progress[0]!= 100);
                printf("Test Mode Complete\n");
                sharedData->progress[0] = 0;
                Mode = 2;
                continue;
            }
            else if (args.input == 0 && activeRequests > 0)
            {
                printf("Warning: Active Requests ongoing so can not enter Test Mode\n\n");
            } else {
                pthread_create(&sendThread, NULL, sendToServer, &args);
                pthread_detach(sendThread);  // So we don't need to join later
                Mode = 1;
                activeRequests++;
            }// Increment active request count
        } else {

            printf("System is busy. Please wait and try again.\n");

        }



        usleep(50000);
    }


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

        if (activeRequests == 0){
            Mode = 0;
        }
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        long elapsed = (currentTime.tv_sec - lastProgressUpdateTime.tv_sec) * 1000 +
                       (currentTime.tv_usec - lastProgressUpdateTime.tv_usec) / 1000;


        if (elapsed >= 500 && Mode == 1) {  // Do not display progress in test mode
            displayProgress(sharedData);
            gettimeofday(&lastProgressUpdateTime, NULL);
        }

        for (int i = 0; i < 10; i++) {
            if (sharedData->serverFlag[i] == 1) {
                printf("Data from server (slot %d): %u\n", i, sharedData->slot[i]);
                if(sharedData->timeElapsed[i] > 0){
                    printf("Slot %d took %ld milliseconds\n", i, sharedData->timeElapsed[i]);
                   sharedData->timeElapsed[i] = 0;
                   activeRequests--;

                }
                sharedData->serverFlag[i] = 0;
            }
        }
        usleep(10000);
//
    }


}



int getInput(sharedMemory *sharedData, int shmID) {
    char input[32];
    int value;

    while (true) {
        printf("Enter a positive integer (or 'q' to quit, '0' for test mode): ");
        fgets(input, sizeof(input), stdin);

        // Remove newline character from input string
        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }

        if (input[0] == 'q' || input[0] == 'Q') {
            cleanupSharedMemory(sharedData, shmID);
            exit(0);
        }

        // Check for Test Mode
        if (input[0] == '0' && input[1] == '\0' && activeRequests == 0) {
            printf("\nEntering Test Mode...\n\n");
            Mode = 2;  // Assuming you have a global or static variable named Mode
            return 0;  // Return 0 to signal test mode
        }

        // Convert to integer and check if it's a positive integer or 0
        value = atoi(input);
        if (value >= 0 && strlen(input) > 0 && strspn(input, "0123456789") == strlen(input)) {
            return value;
        } else {
            printf("Invalid input. Please enter a positive integer or 'q'.\n");
        }
    }
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