#include "shared.h"
#include <stdio.h>
#include <stdlib.h>


int initializeSharedMemory(sharedMemory **sharedData);

uint32_t receiveFromClient(sharedMemory *sharedData);

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data);

void cleanupSharedMemory(sharedMemory *sharedData, int shmID);




int main() {

    int shmID;
    sharedMemory *sharedData = (sharedMemory *)malloc(sizeof(sharedMemory));
    if (sharedData == NULL) {
        perror("Failed to allocate memory for sharedData");
        exit(1);
    }

    shmID = initializeSharedMemory(&sharedData);

    uint32_t data = receiveFromClient(sharedData);
    printf("Received data from client: %u\n", data);
    sendTOClient(sharedData,0, data + 100);

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

    return shmID;
}

uint32_t receiveFromClient(sharedMemory *sharedData) {
    while (sharedData->clientFlag == 0);
    uint32_t data = sharedData->number;
    sharedData->clientFlag = 0;
    return data;
}

void sendTOClient(sharedMemory *sharedData, int slot_index, uint32_t data) {
    while (sharedData->serverFlag[slot_index] == 1);
    sharedData->slot[slot_index] = data;
    sharedData->serverFlag[slot_index] = 1;
}


void cleanupSharedMemory(sharedMemory *sharedData, int shmID) {
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