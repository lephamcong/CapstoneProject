// ===================== scheduler.c =====================
#include "define.h"

uint64_t get_elapsed_ms(struct timespec start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec - start.tv_nsec) / 1000000;
}

uint64_t get_current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

void allocate_tb_size_and_test_scheduler(UEData* ue_data, SchedulerResponse* response) {
    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 100; // Allocate 100 TBSize for each UE
        printf("[Allocate TBSize & Test Scheduler] UE %d: CQI=%d, BSR=%d, Allocated TBSize=%d\n", 
               ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr, response[i].tb_size);
    }
}
int main() {
    // Tạo shm
    int shm_sync = shm_open(SHM_SYNC_TIME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_sync, sizeof(SyncTime));
    SyncTime* sync = mmap(NULL, sizeof(SyncTime), PROT_READ | PROT_WRITE, MAP_SHARED, shm_sync, 0);

    int shm_ue = shm_open(SHM_CQI_BSR, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_ue, sizeof(UEData) * MAX_UE);
    UEData* ue_data = mmap(NULL, sizeof(UEData) * MAX_UE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_ue, 0);

    int shm_scheduler = shm_open(SHM_DCI, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_scheduler, sizeof(SchedulerResponse) * MAX_UE);
    SchedulerResponse* response_data = mmap(NULL, sizeof(SchedulerResponse) * MAX_UE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_scheduler, 0);

    // Tạo semaphore
    sem_t* sem_ue = sem_open(SEM_CQI, O_CREAT, 0666, 0);
    sem_t* sem_scheduler = sem_open(SEM_DCI, O_CREAT, 0666, 0);
    sem_t* sem_sync = sem_open(SEM_SYNC, O_CREAT, 0666, 0);

    // Buffer rieeng
    UEData *ue = (UEData*) malloc(NUM_UE*sizeof(UEData));
    if (ue == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    }

    SchedulerResponse *response = (SchedulerResponse*) malloc(NUM_UE*sizeof(SchedulerResponse));
    if (response == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    }

    sem_wait(sem_ue);
    memcpy(ue, ue_data, sizeof(UEData) * MAX_UE);
    sem_post(sem_ue);

    // Ghi thời gian bắt đầu
    sync->start_time_ms = get_current_time_ms();
    int tti_last = get_elapsed_ms(sync->start_time_ns);
    sem_post(sem_sync);

    printf("[Scheduler] Start sync time: %ld\n", sync->start_time_ns);
    for (int i = 0; i < MAX_UE; i++) {
        printf("[Scheduler] UE %d: CQI=%d, BSR=%d\n", ue_data[i].id, ue_data[i].cqi, ue_data[i].bsr);
    }

    // Gọi hàm cấp phát TBSize và kiểm tra Scheduler
    allocate_tb_size_and_test_scheduler(ue, response_data);
    sleep(1);
    sem_post(sem_scheduler);

    uint64_t tti_now = get_elapsed_ms(sync->start_time_ns);

    while (tti_now <= NUM_TTI) {
        tti_now = get_elapsed_ms(sync->start_time_ns);
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

    // printf("[Scheduler] Started at time: %ld\n", sync->start_time_ns);

    // while (1) {
    //     sem_wait(sem_cqi); // Chờ dữ liệu từ UE

    //     for (int i = 0; i < MAX_UE; i++) {
    //         int tb_size = cqi_data[i].cqi * 10; // giả lập logic
    //         dci_data[i].ue_id = cqi_data[i].ue_id;
    //         dci_data[i].tb_size = tb_size;
    //         dci_data[i].tti = (current_time_ns() - sync->start_time_ns) / TTI_PERIOD_NS;
    //     }

    //     printf("[Scheduler] Sent DCI at TTI %d\n", dci_data[0].tti);
    //     sem_post(sem_dci);
    //     sleep(1000); // 1ms
    // }
    return 0;
}
