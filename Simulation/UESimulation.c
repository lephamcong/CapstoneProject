/**
 * @file UESimulation.c
 * @brief This file contains functions to simulate User Equipment (UE) behavior in 5G NR.
 *
 * The main functionalities include:
 * - Initializing UE data with random values.
 * - Updating UE data based on Transport Block (TB) size.
 * - Sending and receiving data to/from MAC Scheduler.
 *
 * Functions:
 * - void init_data(UEData *ue, int id): Initializes UE data with random values.
 * - void update_data(UEData* ue, int TBSize): Updates UE data based on TB size.   
 *  
 * The main function performs the following steps:
 * - Initializes UE data for all UEs.
 * - Sends data to MAC Scheduler.
 * - Receives data from MAC Scheduler.
 * - Updates UE data based on TB size.
 * - Repeats the process for multiple iterations.
 */
#include "define.h"


void init_data(UEData *ue, int id);
void update_data(UEData* ue, int TBSize);

int main() {
    UEData *ue = (UEData*) malloc(NUM_UE*sizeof(UEData));
    if (ue == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return -1;
    }
    
    for (int i = 0; i < NUM_UE; i++) {
        init_data(&ue[i], i);
    }

    // Send data to MAC Scheduler

    // Receive data from MAC Scheduler
    // Update data

    return 0;
}

void init_data(UEData *ue, int id){
    ue->id = id;
    ue->bsr = rand();
    if (id>0  && id<=4) {
        ue->mcs = rand()%9 + 19;
    }
    else if (id>4 && id<=8){
        ue->mcs = rand()%9 + 10;
    }
    else if (id>8 && id<=12){
        ue->mcs = rand()%10;
    }
    else{
        printf("Invalid UE ID\n");
    }
}

void update_data(UEData* ue, int TBSize){
    ue->bsr = (ue->bsr > TBSize ? ue->bsr - TBSize : 0);
    ue->mcs = ue->mcs + rand()%3 - 1;
    if (ue->mcs < 0) {
        ue->mcs = 0;
    } else if (ue->mcs > 27) {
        ue->mcs = 27;
    }
}