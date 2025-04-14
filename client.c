#include "define.h"

long current_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

void init_data(UEData *ue, int id){
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
    ue->bsr = (ue->bsr > TBSize ? ue->bsr - TBSize : 0);
    ue->cqi = ue->cqi + rand()%3 - 1;
    if (ue->cqi < 0) {
        ue->cqi = 0;
    } else if (ue->cqi > 27) {
        ue->cqi = 27;
    }
}

int main() {
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

    for (int i = 0; i < NUM_UE; i++) {
        init_data(&ue_data[i],i+1);
    }
    sem_post(sem_ue); // Đánh dấu đã gửi dữ liệu đến Scheduler

    sem_wait(sem_sync); // Chờ sync
    long start_ns = sync->start_time_ns;
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

    uint64_t tti_now = get_elapsed_ms(sync->start_time_ns);

    while(tti_now <= NUM_TTI) {
        uint64_t tti_now = get_elapsed_ms(sync->start_time_ns);
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
    // while (1) {
    //     long now = current_time_ns();
    //     int tti = (now - start_ns) / TTI_PERIOD_NS;

    //     for (int i = 0; i < MAX_UE; i++) {
    //         cqi_data[i].ue_id = i;
    //         cqi_data[i].cqi = (i + tti) % 15 + 1;
    //         cqi_data[i].bsr = 50;
    //     }

    //     printf("[UE] Sent CQI at TTI %d\n", tti);
    //     sem_post(sem_cqi);

    //     sem_wait(sem_dci);
    //     for (int i = 0; i < MAX_UE; i++) {
    //         printf("[UE] UE %d received TBSize = %d at TTI = %d\n", dci_data[i].ue_id, dci_data[i].tb_size, dci_data[i].tti);
    //     }

    //     sleep(1000); // 1ms
    // }
    return 0;
}
