#include "shared.h"
#include <stdio.h>
#include <stdlib.h>



void attachToSharedMemory(sharedMemory **sharedData);



void sendToServer(sharedMemory *sharedData, uint32_t data);


uint32_t readFromServer(sharedMemory *sharedData, int slot_index);

uint32_t getInput();

int main() {

    sharedMemory *sharedData;
    uint32_t number;

    attachToSharedMemory(&sharedData);

    number = getInput();

    sendToServer(sharedData,number);
    printf("Data from server (slot 0): %u\n", readFromServer(sharedData, 0));

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


void sendToServer(sharedMemory *sharedData, uint32_t data) {
    while (sharedData->clientFlag == 1);
    sharedData->number = data;
    sharedData->clientFlag = 1;
}


uint32_t readFromServer(sharedMemory *sharedData, int slot_index) {
    while (sharedData->serverFlag[slot_index] == 0);
    uint32_t data = sharedData->slot[slot_index];
    sharedData->serverFlag[slot_index] = 0;
    return data;
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