// ===================== scheduler.c =====================
#include "define.h"

uint64_t get_elapsed_ms(uint64_t start) {
    /*
        Function to calculate elapsed time in milliseconds (ms)
        since the start time.
        
        Parameters:
            start:  The start time in nanoseconds (ns).
        Returns:
            Elapsed time in milliseconds (ms).
    */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint64_t now_ns = (uint64_t)now.tv_sec * 1e9 + now.tv_nsec;
    return (now_ns - start)/1000000;
    
}

uint64_t get_current_time_ns() {
    /*
        Function to get the current time in nanoseconds (ns).
        
        Returns:
            Current time in nanoseconds (ns).
    */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1e9 + ts.tv_nsec;
}

uint64_t get_current_time_ms() {
    /*
        Function to get the current time in milliseconds (ms).
        
        Returns:
            Current time in milliseconds (ms).
    */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

void allocate_tb_size_and_test_scheduler(UEData* ue_data, SchedulerResponse* response) {
    /*
        Function to allocate TBSize and test the scheduler.
        
        Parameters:
            ue_data:    Pointer to UEData structure containing UE information.
            response:   Pointer to SchedulerResponse structure for storing the response.
    */
    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 100;  // Allocate 100 TBSize for each UE
        printf("[Allocate TBSize & Test Scheduler] UE %d: CQI=%d, BSR=%d, Allocated TBSize=%d\n", 
               ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr, response[i].tb_size);
    }
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

    printf("[Scheduler] Start sync time: %ld\n", sync->start_time_ms);
    for (int i = 0; i < MAX_UE; i++) {
        printf("[Scheduler] UE %d: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
    }

    // Allocate TBSize and test scheduler
    allocate_tb_size_and_test_scheduler(ue, response_data);
    sleep(1);
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
