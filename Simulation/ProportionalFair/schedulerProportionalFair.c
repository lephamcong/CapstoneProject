#include "../define.h"

#define TTI_DURATION_NS 1000000L // 1ms = 1,000,000 nanoseconds
#define MAX_TTI_WITHOUT_SCHED 40    // Maximum TTI without scheduling before UE is considered inactive
#define T_c 20                     // Time constant for Proportional Fair scheduling (in TTI)

int TBS[MAX_MCS_INDEX][NUM_RB];

void ProportionalFair(
    UEData *ue_data, 
    SchedulerResponse *response,
    float *avg_throughput,
    int *tti_since_last_sched
);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        LOG_ERROR("Missing scenario file.");
        return 1;
    }
    const char *result_pathfile = argv[1];      // File path for logging results
    int tti_now = 0, tti_last = 0;              // Current and last TTI
    float avg_throughput[NUM_UE] = {0};         // Average throughput for each UE
    int tti_since_last_sched[NUM_UE] = {0};     // TTI since last scheduling for each UE

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

    // Open the result file for logging
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
        // Check if the TTI is not skipping
        if (tti_now > tti_last) {
            if (tti_now - tti_last != 1) {
                LOG_ERROR("Scheduler cannot keep up with TTI");
                LOG_ERROR("Scheduler TTI: %d, TTI last %d\n", tti_now, tti_last);
                return -1;
            }
            LOG_INFO("Run TTI: %d\n", tti_now);
            // If the TTI is a multiple of NUM_TTI_RESEND, receive data from UE and post semaphore
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
                if (sem_post(sem_scheduler_recv) == -1) {
                    LOG_ERROR("Failed to post semaphore for UE data");
                    return 1;
                } else {
                    LOG_OK("Semaphore for UE data posted successfully");
                }
            }
            
            ProportionalFair(ue, response_data, avg_throughput, tti_since_last_sched);

            LOG_OK("Scheduler processed UE data successfully");

            if (sem_post(sem_scheduler_send) == -1) {
                LOG_ERROR("Failed to post semaphore for SchedulerResponse");
                return 1;
            } else {
                LOG_OK("Semaphore for SchedulerResponse posted successfully");
            }

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

void ProportionalFair(UEData *ue_data, SchedulerResponse *response,
                      float *R_k, int *tti_since_last_sched) {
    int scheduled = 0;
    int rb_per_ue = NUM_RB / MAX_UE_PER_TTI;

    int scheduled_ue[NUM_UE] = {0};  

    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 0;
    }

    while (scheduled < MAX_UE_PER_TTI) {
        float max_P_k = -1.0;
        int selected = -1;

        for (int i = 0; i < NUM_UE; i++) {
            if (scheduled_ue[i] || ue_data[i].bsr <= 0 || ue_data[i].cqi <= 0)
                continue;

            int mcs = cqi_to_mcs(ue_data[i].cqi);
            if (mcs < 0 || mcs >= MAX_MCS_INDEX) continue;

            int tb_size = TBS[mcs][rb_per_ue - 1];
            if (tb_size <= 0) continue;

            float r_k = (float)tb_size;
            float P_k;

            if (R_k[i] > 0.0) {
                P_k = r_k / R_k[i];
            } else {
                P_k = r_k;
            }

            if (P_k > max_P_k) {
                max_P_k = P_k;
                selected = i;
            }
        }

        if (selected == -1) break;

        int mcs = cqi_to_mcs(ue_data[selected].cqi);
        int tb_size = TBS[mcs][rb_per_ue - 1];

        response[selected].tb_size = tb_size;
        ue_data[selected].bsr -= tb_size;
        if (ue_data[selected].bsr < 0) ue_data[selected].bsr = 0;

        R_k[selected] = (1 - (float)1 / T_c) * R_k[selected] + (float)1 / T_c * tb_size;

        tti_since_last_sched[selected] = 0; 
        scheduled_ue[selected] = 1;
        scheduled++;
    }

    for (int i = 0; i < NUM_UE; i++) {
        if (!scheduled_ue[i]) {
            tti_since_last_sched[i]++;
            R_k[i] = (1 - (float)1 / T_c) * R_k[i];  // Giảm trung bình theo thời gian
        }
    }
}