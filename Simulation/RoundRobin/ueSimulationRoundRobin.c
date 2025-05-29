#include "Simulation/define.h"

void update_data(UEData* ue, int TBSize);
void get_data_from_file(const char *cqi_filename, const char *bsr_filename, int tti, UEData *ue);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Missing scenario file.\n");
        return 1;
    }
    const char* cqi_pathfile = argv[1];
    const char* bsr_pathfile = argv[2];
    // create shared memory for synchronization
    int shm_sync = shm_open(
        SHM_SYNC_TIME,      // name of the shared memory object 
        O_CREAT | O_RDWR,   // create and open the shared memory object
        0666                // permissions: read and write for all users
    );
    if (shm_sync == -1) {
        LOG_ERROR("Failed to create shared memory for sync");
        return 1;
    } else {
        LOG_OK("Shared memory for sync created successfully");
    }

    // set the size of the shared memory object
    if (ftruncate(shm_sync, sizeof(SyncTime)) == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
    } else {
        LOG_OK("Size for sync shared memory set successfully");
    }

    // map the shared memory object to the process's address space
    SyncTime* sync = mmap(
        NULL,                   // address to start mapping 
        sizeof(SyncTime),       // size of the shared memory object
        PROT_READ | PROT_WRITE, // permissions: read and write
        MAP_SHARED,             // shared memory mapping
        shm_sync,               // file descriptor of the shared memory object
        0                       // offset
    );
    if (sync == MAP_FAILED) {
        LOG_ERROR("Failed to mmap sync");
        return 1;
    } else {
        LOG_OK("Shared memory for sync mapped successfully");
    }

    // create shared memory for UE data
    int shm_ue = shm_open(
        SHM_CQI_BSR,        // name of the shared memory object 
        O_CREAT | O_RDWR,   // create and open the shared memory object
        0666                // permissions: read and write for all users
    );
    if (shm_ue == -1) {
        LOG_ERROR("Failed to create shared memory for UE data");
        return 1;
    } else {
        LOG_OK("Shared memory for UE data created successfully");
    }

    // set the size of the shared memory object
    if (ftruncate(shm_ue, sizeof(UEData) * MAX_UE) == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
    } else {
        LOG_OK("Size for UE data shared memory set successfully");
    }

    // map the shared memory object to the process's address space
    UEData* ue_data = mmap(
        NULL,                       // address to start mapping 
        sizeof(UEData) * MAX_UE,    // size of the shared memory object
        PROT_READ | PROT_WRITE,     // permissions: read and write
        MAP_SHARED,                 // shared memory mapping
        shm_ue,                     // file descriptor of the shared memory object
        0                           // offset
    );
    if (ue_data == MAP_FAILED) {
        LOG_ERROR("Failed to mmap ue_data");
        return 1;
    } else {
        LOG_OK("Shared memory for UE data mapped successfully");
    }

    // create shared memory for SchedulerResponse
    int shm_scheduler = shm_open(
        SHM_DCI,            // name of the shared memory object 
        O_CREAT | O_RDWR,   // create and open the shared memory object
        0666                // permissions: read and write for all users
    );
    if (shm_scheduler == -1) {
        LOG_ERROR("Failed to create shared memory for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Shared memory for SchedulerResponse created successfully");
    }

    // set the size of the shared memory object
    if (ftruncate(shm_scheduler, sizeof(SchedulerResponse) * MAX_UE) == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
    }   else {
        LOG_OK("Size for SchedulerResponse shared memory set successfully");
    }

    // map the shared memory object to the process's address space
    SchedulerResponse* response_data = mmap(
        NULL,                               // address to start mapping 
        sizeof(SchedulerResponse) * MAX_UE, // size of the shared memory object
        PROT_READ | PROT_WRITE,             // permissions: read and write
        MAP_SHARED,                         // shared memory mapping
        shm_scheduler,                      // file descriptor of the shared memory object
        0                                   // offset
    );
    if (response_data == MAP_FAILED) {
        LOG_ERROR("Failed to mmap response_data");
        return 1;
    } else {
        LOG_OK("Shared memory for SchedulerResponse mapped successfully");
    }

    // create semaphore
    sem_t* sem_ue_send = sem_open(
        SEM_UE_SEND,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_ue_send == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data created successfully");
    }

    sem_t* sem_ue_recv = sem_open(
        SEM_UE_RECV,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_ue_recv == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data created successfully");
    }

    sem_t* sem_scheduler_send = sem_open(
        SEM_SCHEDULER_SEND,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_scheduler_send == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse created successfully");
    }
    sem_t* sem_scheduler_recv = sem_open(
        SEM_SCHEDULER_RECV,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_scheduler_recv == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse created successfully");
    }

    sem_t* sem_sync = sem_open(
        SEM_SYNC,   // name of the semaphore 
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_sync == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for sync");
        return 1;
    } else {
        LOG_OK("Semaphore for sync created successfully");
    }

    // Buffer of response_data
    SchedulerResponse *response = (SchedulerResponse*) malloc(NUM_UE*sizeof(SchedulerResponse));
    if (response == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    } else {
        LOG_OK("Memory allocation for SchedulerResponse successful");
    }

    LOG_OK("Ready...");
    sync->start_time_ms = get_current_tti_frame();
    if (sem_post(sem_sync) == -1) {
        LOG_ERROR("Failed to post sync semaphore");
        return 1;
    } else {
        LOG_OK("Semaphore for sync posted successfully");
    }
    int tti_now = 0, tti_last = 0;

    while (tti_now < NUM_TTI) {
        tti_now = get_elapsed_tti_frame(sync->start_time_ms);
        if (tti_now > tti_last) {
            if (tti_now - tti_last != 1) {
                LOG_ERROR("Scheduler cannot keep up with TTI");
                LOG_ERROR("Scheduler TTI: %d, TTI last %d\n", tti_now, tti_last);
                return -1;
            }

            LOG_INFO("Run TTI: %d\n", tti_now);
            if (tti_now % NUM_TTI_RESEND == 1) {
                get_data_from_file(cqi_pathfile, bsr_pathfile, tti_now, ue_data);
                if (sem_post(sem_ue_send) == -1) {
                    LOG_ERROR("Failed to post semaphore for UE data");
                    return 1;
                } else {
                    LOG_OK("Semaphore for UE data posted successfully");
                }
                // printf("[Send] Start sent at TTI: %d\n", tti_now);
                // for (int i = 0; i < MAX_UE; i++) {
                //     printf("[Send] UE %d sent data: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
                // }
                if (sem_wait(sem_scheduler_recv) == -1) {
                    LOG_ERROR("Failed to wait for UE data semaphore");
                    return 1;
                } else {
                    LOG_OK("Scheduler received UE data successfully");
                }
            }

            if (sem_wait(sem_scheduler_send) == -1) {
                LOG_ERROR("Failed to wait for UE data semaphore");
                return 1;
            } else {
                LOG_OK("Scheduler received UE data successfully");
            }
            if (memcpy(response, response_data, sizeof(SchedulerResponse) * MAX_UE) == NULL) {
                LOG_ERROR("Failed to copy response data");
                return 1;
            } else {
                LOG_OK("Scheduler received response data successfully");
            }

            // printf("[Receive] Start receive at TTI: %d\n", tti_now);
            // for (int i = 0; i < MAX_UE; i++) {
            //     printf("[Receive] UE %d receive data: TBSize=%d\n", response[i].id, response[i].tb_size);
            // }
            for (int i = 0; i < MAX_UE; i++) {
                update_data(&ue_data[i], response[i].tb_size);
            }
            // printf("[Update] Scheduler updated UE data successfully\n");
            // for (int i = 0; i < MAX_UE; i++) {
            //     printf("[Update] UE %d after upgrade: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
            // }
            if (sem_post(sem_ue_recv) == -1) {
                LOG_ERROR("Failed to post semaphore for SchedulerResponse");
                return 1;
            } else {
                LOG_OK("Semaphore for SchedulerResponse posted successfully");
            }
                
            tti_last = tti_now;
        }
    }

    //  free allocated memory
    free(response);
    response = NULL;
    if (munmap(ue_data, sizeof(UEData) * MAX_UE) == -1) {
        LOG_ERROR("Failed to unmap ue_data");
        return 1;
    } else {
        LOG_OK("Shared memory for UE data unmapped successfully");
    }
    if (munmap(response_data, sizeof(SchedulerResponse) * MAX_UE) == -1) {
        LOG_ERROR("Failed to unmap response_data");
        return 1;
    } else {
        LOG_OK("Shared memory for SchedulerResponse unmapped successfully");
    }
    if (munmap(sync, sizeof(SyncTime)) == -1) {
        LOG_ERROR("Failed to unmap sync");
        return 1;
    } else {
        LOG_OK("Shared memory for sync unmapped successfully");
    }
    if (close(shm_sync) == -1) {
        LOG_ERROR("Failed to close shared memory for sync");
        return 1;
    } else {
        LOG_OK("Shared memory for sync closed successfully");
    }
    if (close(shm_ue) == -1) {
        LOG_ERROR("Failed to close shared memory for UE data");
        return 1;
    } else {
        LOG_OK("Shared memory for UE data closed successfully");
    }
    if (close(shm_scheduler) == -1) {
        LOG_ERROR("Failed to close shared memory for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Shared memory for SchedulerResponse closed successfully");
    }
    if (sem_close(sem_ue_send) == -1) {
        LOG_ERROR("Failed to close semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data closed successfully");
    }
    if (sem_close(sem_ue_recv) == -1) {
        LOG_ERROR("Failed to close semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data closed successfully");
    }
    if (sem_close(sem_scheduler_send) == -1) {
        LOG_ERROR("Failed to close semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse closed successfully");
    }
    if (sem_close(sem_scheduler_recv) == -1) {
        LOG_ERROR("Failed to close semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse closed successfully");
    }
    if (sem_close(sem_sync) == -1) {
        LOG_ERROR("Failed to close semaphore for sync");
        return 1;
    } else {
        LOG_OK("Semaphore for sync closed successfully");
    }

    return 0;
}

void update_data(UEData* ue, int TBSize){
    /*
        Function to update UE data.
            UEData structure contains:
                - id: UE ID
                - cqi: Channel Quality Indicator
                - bsr: Buffer Status Report
        Parameters:
            ue: Pointer to UEData structure to be updated.
            TBSize: Size of the Transport Block.
    */
    ue->bsr = (ue->bsr > TBSize ? ue->bsr - TBSize : 0);
    // ue->cqi = ue->cqi + rand()%3 - 1;
    // if (ue->cqi < 0) {
        // ue->cqi = 0;
    // } else if (ue->cqi > 27) {
        // ue->cqi = 27;
    // }
}

void get_data_from_file(const char *cqi_filename, const char *bsr_filename, int tti, UEData *ue){
    /*
        Function to read data from files and update UE data.
            UEData structure contains:
                - id: UE ID
                - cqi: Channel Quality Indicator
                - bsr: Buffer Status Report
        Parameters:
            cqi_filename: Name of the file containing CQI data.
            bsr_filename: Name of the file containing BSR data.
            tti: TTI value to read data for.
            ue: Pointer to UEData structure to be updated.
    */
    FILE *cqi_file = fopen(cqi_filename, "r");
    FILE *bsr_file = fopen(bsr_filename, "r");
    if (cqi_file == NULL || bsr_file == NULL) {
        LOG_ERROR("Error opening file for reading");
        return;
    }
    char line1[MAX_LINE_LENGTH],line2[MAX_LINE_LENGTH];
    int current_tti  = 1;
    int count_UE;

    while (fgets(line1, sizeof(line1), cqi_file) && fgets(line2, sizeof(line2), bsr_file)) {
        if (current_tti == tti + 1) {
            char *token1 = strtok(line1, ",");
            count_UE = 0;
            while (token1 != NULL) {
                ue[count_UE].id = count_UE + 1; 
                ue[count_UE].cqi = atoi(token1); // Convert string to integer
                count_UE++;
                token1 = strtok(NULL, ",");
            }
            char *token2 = strtok(line2, ",");
            count_UE = 0;
            while (token2 != NULL) {
                ue[count_UE].bsr += atoi(token2); // Convert string to integer
                count_UE++;
                token2 = strtok(NULL, ",");
            }
        }
        current_tti++;
    }
    // for (int i = 0; i < MAX_UE; i++) {
    //     printf("[In get data function] UE %d: CQI=%d, BSR=%d\n", ue[i].id, ue[i].cqi, ue[i].bsr);
    // }
    fclose(cqi_file);
    fclose(bsr_file);
}