#include "define.h"

long current_time_ns() {
    /*
        Function to get the current time in nanoseconds (ns).
        
        Returns:
            Current time in nanoseconds (ns).
    */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

long current_time_ms() {
    /*
        Function to get the current time in milliseconds (ms).
        
        Returns:
            Current time in milliseconds (ms).
    */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

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

void init_data(UEData *ue, int id){
    /*
        Function to initialize UE data.
            UEData structure contains:
                - id: UE ID
                - cqi: Channel Quality Indicator
                - bsr: Buffer Status Report
            id from 1 to 12
            cqi from 0 to 27
                id 1-4: cqi from 19 to 27
                id 5-8: cqi from 10 to 18
                id 9-12: cqi from 0 to 9
        Parameters:
            ue: Pointer to UEData structure to be initialized.
            id: ID of the UE.
    */
    ue->id = id;
    ue->bsr = rand();
    if (id>0  && id<=4) {
        ue->cqi = rand()%9 + 19;
    }
    else if (id>4 && id<=8){
        ue->cqi = rand()%9 + 10;
    }
    else if (id>8 && id<=12){
        ue->cqi = rand()%10;
    }
    else{
        printf("Invalid UE ID\n");
    }
    ue->bsr = rand()%10000;
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
    ue->cqi = ue->cqi + rand()%3 - 1;
    if (ue->cqi < 0) {
        ue->cqi = 0;
    } else if (ue->cqi > 27) {
        ue->cqi = 27;
    }
}

int main() {
    // create shared memory for synchronization
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
    if (ftruncate(shm_sync, sizeof(SyncTime)) == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
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
    if (ftruncate(shm_ue, sizeof(UEData) * MAX_UE) == -1) {
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
    if (ftruncate(shm_scheduler, sizeof(SchedulerResponse) * MAX_UE) == -1) {
        LOG_ERROR("Failed to set size for shared memory");
        return 1;
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
        0           // initial value of the semaphore
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

    for (int i = 0; i < NUM_UE; i++) {
        init_data(&ue_data[i],i+1);
    }
    sem_post(sem_ue);   // signals that UE data is ready to be sent

    sem_wait(sem_sync); // wait for sync signal
    
    long start_ns = sync->start_time_ms;
    printf("[UE] Start sync time: %ld\n", start_ns);


    for (int i = 0; i < MAX_UE; i++) {
        printf("[UE] UE %d: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
    }

    sem_wait(sem_scheduler); 
    for (int i = 0; i < MAX_UE; i++) {
        ue_data[i].bsr = (ue_data[i].bsr > response_data[i].tb_size ? ue_data[i].bsr - response_data[i].tb_size : 0);
    }

    for (int i = 0; i < MAX_UE; i++) {
        printf("[UE received] UE %d: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
    }

    uint64_t tti_now = get_elapsed_ms(sync->start_time_ms);

    while(tti_now <= NUM_TTI) {
        uint64_t tti_now = get_elapsed_ms(sync->start_time_ms);
        if (tti_now % NUM_TTI_RESEND == 0) {
            // start_ns = tti_now;
            // for (int i = 0; i < NUM_UE; i++) {
            //     init_data(&ue_data[i],i+1); 
            // }
            sem_post(sem_ue); // Đánh dấu đã gửi dữ liệu đến Scheduler
            printf("[UE] Start sent at TTI: %ld\n", tti_now);
            for (int i = 0; i < MAX_UE; i++) {
                printf("[UE] UE %d sent data: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
            }
        }

        sem_wait(sem_scheduler); 
        for (int i = 0; i < MAX_UE; i++) {
            update_data(&ue_data[i], response_data[i].tb_size);
        }

        for (int i = 0; i < MAX_UE; i++) {
            printf("[UE received] UE %d after upgrade: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
        }
    }
    

    return 0;
}