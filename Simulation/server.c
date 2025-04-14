#include "define.h"
#include "algorithm/roundRobin.c"
#include "algorithm/MaxC_I.c"
#include "algorithm/PF.c"

int TBS[MAX_MCS_INDEX][NUM_RB];

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
    FILE *file = fopen("./TBSArray.csv", "r");
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

void RoundRobin() {

}


int main() {
    // create shared memory
    int shm_sync = shm_open(
        SHM_SYNC_TIME,      // name of the shared memory object
        O_CREAT | O_RDWR,   // create and open the shared memory object
        0666                // permissions: read and write for all users
    );
    if (shm_sync == -1) {
        LOG_ERROR("Failed to create shared memory for sync");
        return 1;
    }

    // set the size of the shared memory object
    ftruncate(shm_sync, sizeof(SyncTime)); // set the size of the shared memory object
    if (shm_sync == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
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
    }

    // set the size of the shared memory object
    ftruncate(shm_ue, sizeof(UEData) * MAX_UE);
    if (shm_ue == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
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
    }

    // set the size of the shared memory object
    ftruncate(shm_scheduler, sizeof(SchedulerResponse) * MAX_UE);
    if (shm_scheduler == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
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
    }

    // create semaphore
    sem_t* sem_ue = sem_open(
        SEM_CQI,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_ue == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for UE data");
        return 1;
    }

    sem_t* sem_scheduler = sem_open(
        SEM_DCI,    // name of the semaphore
        O_CREAT,    // create the semaphore if it doesn't exist
        0666,       // permissions: read and write for all users
        0           // initial value of the semaphore
    );
    if (sem_scheduler == SEM_FAILED) {
        LOG_ERROR("Failed to create semaphore for SchedulerResponse");
        return 1;
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
    }

    // Buffer of ue_data
    UEData *ue = (UEData*) malloc(NUM_UE*sizeof(UEData));
    if (ue == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    }

    // Buffer of response_data
    SchedulerResponse *response = (SchedulerResponse*) malloc(NUM_UE*sizeof(SchedulerResponse));
    if (response == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    }

    sem_wait(sem_ue);   // wait for UE data
    memcpy(ue, ue_data, sizeof(UEData) * MAX_UE);   // copy data from shared memory to local buffer
    sem_post(sem_ue);   // signal that data has been copied

    // save start time
    sync->start_time_ms = get_current_time_ms();
    int tti_last = get_elapsed_ms(sync->start_time_ms);
    sem_post(sem_sync);

    printf("[Scheduler] Start sync time: %s\n", format_time_str(sync->start_time_ms));  
    for (int i = 0; i < MAX_UE; i++) {
        printf("[Scheduler] UE %d: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
    }

    // Allocate TBSize and test scheduler
    allocate_tb_size_and_test_scheduler(ue, response_data);
    sem_post(sem_scheduler);

    uint64_t tti_now = get_elapsed_ms(sync->start_time_ms);

    while (tti_now <= NUM_TTI) {
        tti_now = get_elapsed_ms(sync->start_time_ms);
        if (tti_now > tti_last) {
            tti_last = tti_now;
            if(sem_trywait(sem_ue) == 0) {
                memcpy(ue, ue_data, sizeof(UEData) * MAX_UE);
                printf("[Scheduler] Received UE data at TTI %ld\n", tti_now);
                sem_post(sem_ue);
            };

            allocate_tb_size_and_test_scheduler(ue, response_data);
            sem_post(sem_scheduler);
            printf("[Scheduler] Sent DCI at TTI %ld\n", tti_now);
            for (int i = 0; i < MAX_UE; i++) {
                printf("[Scheduler] UE %d received TBSize = %d at TTI = %ld\n", response_data[i].id, response_data[i].tb_size, tti_now);
            }
        }
    }

    return 0;
}
