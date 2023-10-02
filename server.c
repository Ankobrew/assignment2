#include "shared.h"
#include <stdio.h>
#include <stdlib.h>

shared_data_t *shared_data;

void initialize_shared_memory() {
    int shmid = shmget(SHM_KEY, sizeof(shared_data_t), 0666 | IPC_CREAT);
    if (shmid == -1) {
        perror("Failed to create shared memory segment");
        exit(1);
    }
    shared_data = (shared_data_t *) shmat(shmid, NULL, 0);
}
// Function to read data sent by the client
uint32_t receive_from_client() {
    while (shared_data->clientFlag == 0);  // Wait until data is available from the client
    uint32_t data = shared_data->number;   // Read the data
    shared_data->clientFlag = 0;           // Reset the clientFlag
    return data;
}

// Function to send data back to the client
void send_to_client(int slot_index, uint32_t data) {
    while (shared_data->serverFlag[slot_index] == 1);   // Wait until the client has read the previous data for this slot
    shared_data->slot[slot_index] = data;               // Write data to the slot
    shared_data->serverFlag[slot_index] = 1;            // Set the serverFlag for this slot to signal data availability
}

int main() {
    initialize_shared_memory();

    // Sample code for testing the functions
    uint32_t data = receive_from_client();
    printf("Received data from client: %u\n", data);
    send_to_client(0, data + 100);

    return 0;
}
