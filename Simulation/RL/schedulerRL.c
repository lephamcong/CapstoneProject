#include "define.h"

#define TTI_DURATION_NS 1000000L // 1ms = 1,000,000 nanoseconds


int TBS[MAX_MCS_INDEX][NUM_RB];

int cqi_to_mcs(int cqi);    // Mapping CQI to MCS index
void TBS_Table();           // Load TBS table from CSV file
void RoundRobin(UEData* ue_data, SchedulerResponse *response); // Round Robin scheduling algorithm
void log_tbsize(FILE *log_file, int tti, SchedulerResponse *response, int num_ue);
void MaxCQI(UEData *ue_data, SchedulerResponse *response);
void ProportionalFair(UEData *ue_data, SchedulerResponse *response,
                      float *avg_throughput, int *tti_since_last_sched);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        LOG_ERROR("Missing scenario file.");
        return 1;
    }
    const char *result_pathfile = argv[1];
    const char *type_scheduler = argv[2];
    enum SCHEDULER_TYPE scheduler_type;
    if (strcmp(type_scheduler,"rr") == 0) scheduler_type = ROUND_ROBIN; // Default scheduler type
    else if (strcmp(type_scheduler,"max_cqi") == 0) scheduler_type = MAX_CQI;
    else if (strcmp(type_scheduler,"pf") == 0) scheduler_type = PROPORTIONAL_FAIR;
    else if (strcmp(type_scheduler,"q_learning") == 0) scheduler_type = Q_LEARNING;
    else {
        LOG_ERROR("Invalid scheduler type");
        return 1;
    }
    int tti_now = 0, tti_last = 0;
    float avg_throughput[NUM_UE] = {0};         // Trung bình tốc độ ban đầu
            int tti_since_last_sched[NUM_UE] = {0};

    // Define a timespec structure for nanosleep with a timeout of 400 microseconds
    struct timespec req_timeout = {.tv_sec = 0, .tv_nsec = 400000}; 

    // create shared memory
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
    ftruncate(shm_sync, sizeof(SyncTime)); // set the size of the shared memory object
    if (shm_sync == -1) {
        LOG_ERROR("Failed to set size for shared memory for sync");
        return 1;
    } else {
        LOG_OK("Size for sync shared memory set successfully");
    }

    // map the shared memory object to the process's address space
    SyncTime* sync = mmap(
        NULL,                   // address to start mapping
        sizeof(SyncTime),       // size of the shared memory object = ftruncate
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
    ftruncate(shm_ue, sizeof(UEData) * MAX_UE);
    if (shm_ue == -1) {
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
    ftruncate(shm_scheduler, sizeof(SchedulerResponse) * MAX_UE);
    if (shm_scheduler == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
    } else {
        LOG_OK("Size for SchedulerResponse shared memory set successfully");
    }
    // map the shared memory object to the process's address space
    SchedulerResponse* response_data = mmap(
        NULL,                                   // address to start mapping
        sizeof(SchedulerResponse) * MAX_UE,     // size of the shared memory object
        PROT_READ | PROT_WRITE,                 // permissions: read and write
        MAP_SHARED,                             // shared memory mapping
        shm_scheduler,                          // file descriptor of the shared memory object
        0                                       // offset
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
        0           //initial value of the semaphore
    );
    if (sem_sync == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for sync");
        return 1;
    } else {
        LOG_OK("Semaphore for sync created successfully");
    }

    // Buffer of ue_data
    UEData *ue = (UEData*) malloc(NUM_UE*sizeof(UEData));
    if (ue == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    } else {
        LOG_OK("Memory allocation for UE data successful");
    }

    // Load TBS table from CSV file
    TBS_Table();

    FILE *result_file = fopen(result_pathfile, "w");
    if (result_file == NULL) {
        LOG_ERROR("Error opening log file");
        return 1;
    } else {
        LOG_OK("Log file opened successfully");
    }
    fprintf(result_file, "TTI");
    for (int i = 0; i < NUM_UE; i++) {
        fprintf(result_file, ",UE%d", i);
    }

    LOG_OK("Ready...");

    if (sem_wait(sem_sync) == -1) {
        LOG_ERROR("Failed to wait for sync semaphore");
        return 1;
    } else {
        LOG_OK("Scheduler started successfully");
    }
    LOG_INFO("Scheduler start sync time: %s\n", format_time_str(sync->start_time_ms));
    
    // Loop TTI
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
                if (sem_wait(sem_ue_send) == -1) {
                    LOG_ERROR("Failed to wait for UE data semaphore");
                    return 1;
                } else {
                    LOG_OK("Scheduler received UE data successfully");
                }
                // Copy data from shared memory to ue_data
                if (memcpy(ue, ue_data, sizeof(UEData) * NUM_UE) == NULL) {
                    LOG_ERROR("Failed to copy data from shared memory to ue_data");
                    return 1;
                } else {
                    LOG_OK("Data copied from shared memory to ue_data successfully");
                }
                LOG_OK("Scheduler received UE data successfully");
                // printf("[Receive] Scheduler received UE data successfully\n");
                // for (int i = 0; i < NUM_UE; i++) {
                //     printf("[Receive] UE sent data at TTI %d, ID: %d, CQI: %d, BSR: %d\n", tti_now, ue[i].id, ue[i].cqi, ue[i].bsr);
                // }
                if (sem_post(sem_scheduler_recv) == -1) {
                    LOG_ERROR("Failed to post semaphore for UE data");
                    return 1;
                } else {
                    LOG_OK("Semaphore for UE data posted successfully");
                }
            }
            
            switch (scheduler_type) {
                case ROUND_ROBIN:
                    RoundRobin(ue, response_data);
                    break;
                case MAX_CQI:
                    MaxCQI(ue, response_data);
                    break;
                case PROPORTIONAL_FAIR:
                    ProportionalFair(ue, response_data, avg_throughput, tti_since_last_sched);
                    break;
                case Q_LEARNING:
                    //
                    break;
                default:
                    LOG_ERROR("Invalid scheduler type");
                    return 1;
            }
            LOG_OK("Scheduler processed UE data successfully");

            if (sem_post(sem_scheduler_send) == -1) {
                LOG_ERROR("Failed to post semaphore for SchedulerResponse");
                return 1;
            } else {
                LOG_OK("Semaphore for SchedulerResponse posted successfully");
                // printf("[Send] Scheduler sent response data successfully\n");
                // for (int i = 0; i < NUM_UE; i++) {
                //     printf("[Send] TTI %d for UE %d sent data: TBSize=%d\n",tti_now, response_data[i].id, response_data[i].tb_size);
                // }
            }
            // printf("[Update] Scheduler updated UE data successfully\n");
            // for (int i = 0; i < NUM_UE; i++) {
            //     printf("[Updated] UE %d after upgrade: CQI=%d, BSR=%d\n", ue[i].id, ue[i].cqi, ue[i].bsr);
            // }

            log_tbsize(result_file, tti_now, response_data, NUM_UE);

            if (sem_wait(sem_ue_recv) == -1) {
                LOG_ERROR("Failed to wait for SchedulerResponse semaphore");
                return 1;
            } else {
                LOG_OK("Scheduler received response data successfully");
            }

            tti_last = tti_now;
        }
    }

    // Free allocated memory
    free(ue);
    ue = NULL;
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


int cqi_to_mcs(int cqi) {
    /*
        Function to convert CQI to MCS index.
        
        Parameters:
            cqi: CQI value.
        
        Returns:
            MCS index corresponding to the given CQI.
    */
    if (cqi == 1) return 0;
    else if (cqi == 2) return 2;
    else if (cqi == 3) return 4;
    else if (cqi == 4) return 6;
    else if (cqi == 5) return 8;
    else if (cqi == 6) return 10;
    else if (cqi == 7) return 12;
    else if (cqi == 8) return 14;
    else if (cqi == 9) return 16;
    else if (cqi == 10) return 19;
    else if (cqi == 11) return 21;
    else if (cqi == 12) return 23;
    else if (cqi == 13) return 24;
    else if (cqi == 14) return 25;
    else if (cqi == 15) return 27;
    else return -1; // Invalid CQI
}

// Function to load TBS (Transport Block Size) table from CSV file
void TBS_Table() {
    /*
        Function to load TBS (Transport Block Size) table from CSV file.
        
        Returns:
            None
    */
    FILE *file = fopen(TBS_FILE, "r");
    if (file == NULL) {
        LOG_ERROR("Error opening TBS file");
        exit(EXIT_FAILURE);
    }
    // Read the TBS table from the CSV file
    char line[MAX_LINE_LENGTH];
    int i = 0;

    fgets(line, sizeof(line), file); // skip the header line

    while (fgets(line, sizeof(line), file) && i < MAX_MCS_INDEX) {
        char *token = strtok(line, ",");  // skip the first column
        token = strtok(NULL, ",");

        int j = 0;
        while (token != NULL && j < NUM_RB) {
            TBS[i][j] = atoi(token);
            token = strtok(NULL, ",");
            j++;
        }

        if (j != NUM_RB) {
            fprintf(
                stderr, 
                COLOR_YELLOW "[WARN ] [%s:%d] Warning: Row %d has incorrect number of columns" COLOR_RESET "\n", 
                __FILE__, 
                __LINE__, 
                i
            );
        }

        i++;
    }

    if (i != MAX_MCS_INDEX) {
        fprintf(
            stderr, 
            COLOR_YELLOW "[WARN ] [%s:%d] Warning: File has incorrect number of rows" COLOR_RESET "\n", 
            __FILE__, 
            __LINE__
        );
    } else {
        LOG_OK("TBS table loaded successfully");
    }

    fclose(file);
}

void log_tbsize(FILE *log_file, int tti, SchedulerResponse *response, int num_ue) {
    /*
        Function to log the TB size for each UE at a given TTI.
        
        Parameters:
            log_file: Pointer to the log file.
            tti: TTI value.
            response: Pointer to the SchedulerResponse structure.
            num_ue: Number of UEs.
        
        Returns:
            None
    */
    fprintf(log_file, "\n%d", tti);
    for (int i = 0; i < num_ue; i++) {
        fprintf(log_file, ",%d", response[i].tb_size);
    }
};

