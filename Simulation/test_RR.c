#include <stdio.h>
#include <stdlib.h>

#define NUM_UE 12
#define NUM_RB 100
#define MAX_MCS_INDEX 28
#define MAX_UE_PER_TTI 4

typedef struct {
    int id;
    int cqi;
    int bsr;
} UEData;

typedef struct {
    int id;
    int tb_size;
} SchedulerResponse;

// Giả lập bảng TBS đơn giản: tb_size = mcs * rb * 10
int TBS[MAX_MCS_INDEX][NUM_RB];

// CQI → MCS đơn giản
int cqi_to_mcs(int cqi) {
    if (cqi < 1 || cqi > 15) return -1;
    return cqi + cqi % 2; // dùng mapping gần đúng
}

void RoundRobin(UEData *ue_data, SchedulerResponse *response) {
    static int last_index = 0;
    int scheduled = 0;
    int rb_per_ue = NUM_RB / MAX_UE_PER_TTI;

    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 0;
    }

    for (int i = 0; i < NUM_UE && scheduled < MAX_UE_PER_TTI; i++) {
        int idx = (last_index + i) % NUM_UE;
        int cqi = ue_data[idx].cqi;
        int mcs = cqi_to_mcs(cqi);
        if (mcs < 0 || mcs >= MAX_MCS_INDEX) {
            continue;
        }

        int tb_size = TBS[mcs][rb_per_ue - 1];
        if (ue_data[idx].bsr != 0) {
            response[idx].id = ue_data[idx].id;
            response[idx].tb_size = tb_size;
            ue_data[idx].bsr -= tb_size;
            if (ue_data[idx].bsr < 0) ue_data[idx].bsr = 0;
            scheduled++;
        }
    }

    last_index = (last_index + scheduled) % NUM_UE;
}

void init_TBS() {
    for (int mcs = 0; mcs < MAX_MCS_INDEX; mcs++) {
        for (int rb = 0; rb < NUM_RB; rb++) {
            TBS[mcs][rb] = (mcs + 1) * (rb + 1) * 2; // mô phỏng TBSize
        }
    }
}

int main() {
    UEData ue[NUM_UE];
    SchedulerResponse response[NUM_UE];

    init_TBS();

    for (int i = 0; i < NUM_UE; i++) {
        ue[i].id = i + 1;
        ue[i].cqi = 5 + (i % 10);       // CQI từ 5 đến 14
        ue[i].bsr = 5000 + rand() % 5000; // BSR từ 5k - 10k
    }

    printf("=== Before Scheduling ===\n");
    for (int i = 0; i < NUM_UE; i++) {
        printf("UE %d: CQI=%d, BSR=%d\n", ue[i].id, ue[i].cqi, ue[i].bsr);
    }

    RoundRobin(ue, response);

    printf("\n=== After Scheduling ===\n");
    for (int i = 0; i < NUM_UE; i++) {
        printf("UE %d: TBSize=%d, New BSR=%d\n", response[i].id, response[i].tb_size, ue[i].bsr);
    }

    return 0;
}
