#include "shared.h"
#include <stdio.h>
#include <stdlib.h>

shared_data_t *shared_data;

void attach_to_shared_memory() {
    int shmid = shmget(SHM_KEY, sizeof(shared_data_t), 0666);
    if (shmid == -1) {
        perror("Failed to find shared memory segment. Is the server running?");
        exit(1);
    }
    shared_data = (shared_data_t *) shmat(shmid, NULL, 0);
}

// Function to send data to the server
void send_to_server(uint32_t data) {
    while (shared_data->clientFlag == 1);  // Wait until the server has read the previous data
    shared_data->number = data;            // Write data to shared memory
    shared_data->clientFlag = 1;           // Set the clientFlag to signal data availability
}

// Function to read data from the server
uint32_t read_from_server(int slot_index) {
    while (shared_data->serverFlag[slot_index] == 0);  // Wait until data is available
    uint32_t data = shared_data->slot[slot_index];     // Read the data
    shared_data->serverFlag[slot_index] = 0;           // Reset the serverFlag for that slot
    return data;
}

int main() {
    attach_to_shared_memory();

    // Sample code for testing the functions
    send_to_server(12345);
    printf("Data from server (slot 0): %u\n", read_from_server(0));

    return 0;
}