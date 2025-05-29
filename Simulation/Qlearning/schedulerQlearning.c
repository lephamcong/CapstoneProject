#include "Simulation/define.h"

#define SIZE_OF_ACTION_SPACE 2          // Size of the action space (number of actions)
#define SIZE_OF_STATE_SPACE 107653      // Size of the state space (number of states)
#define GAMMA 0.9f                      // Discount factor for future rewards
#define ALPHA 0.1f                      // Learning rate for Q-learning
#define EPSILON 0.1f                    // Exploration rate for epsilon-greedy policy

int TBS[MAX_MCS_INDEX][NUM_RB];
int action_index;

typedef struct {
    int cqi_group[3];   // Array to hold CQI groups for each UE
    int bsr_group[2];   // Array to hold BSR groups for each UE
    int delay_group[3]; // Array to hold delay groups for each UE    
} State;

typedef struct {
    float throughput;
    float fairness;
    float reward;
} Reward;

// Function to implement Q-learning algorithm for scheduling
void classify_state(
    UEData *ue, 
    int *delay, 
    State *state
);
int state_to_index(State *state); // Convert state to index
int select_action_epsilon_greedy(
    int state_idx, 
    double epsilon, 
    float (*Q_table)[SIZE_OF_ACTION_SPACE]
);
void schedule_by_action(
    UEData *ue, 
    int action_idx, 
    SchedulerResponse *response, 
    int *delay);
double calculate_throughput(
    SchedulerResponse *response, 
    int num_ue);
double calculate_fairness(int *ue_tbsize_allocated);
double calculate_reward(
    double throughput, 
    double fairness
);
void update_q_table(
    int state_idx, 
    int action_idx, 
    Reward *reward,
    int next_state_idx, 
    double alpha, 
    double gamma, 
    float (*Q_table)[SIZE_OF_ACTION_SPACE]
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

    State *state = (State *)malloc(sizeof(State));          // Allocate memory for the current state
    State *state_next = (State*) malloc(sizeof(State));     // Allocate memory for the next state

    Reward *reward = (Reward *)malloc(sizeof(Reward));          // Allocate memory for the reward

    float (*QTable)[SIZE_OF_ACTION_SPACE] = malloc(SIZE_OF_STATE_SPACE * sizeof(*QTable));
    if (!QTable) {
        LOG_ERROR("Don't have enough memory for QTable");
        return 1;
    } 

    // Initialize QTable with zeros
    for (int i = 0; i < SIZE_OF_STATE_SPACE; i++) {
        for (int j = 0; j < SIZE_OF_ACTION_SPACE; j++) {
            QTable[i][j] = 0.0f; // Initialize Q-table with zeros
        }
    }

    // Initialize delay array
    int ueDelay[NUM_UE] = {0};                  // Delay for each UE
    int ueTBSize_allocated[NUM_UE] = {0};       // Transport Block Size allocated for each UE

    int tti_now = 0, tti_last = 0;
    float averageThroughput[NUM_UE] = {0};      // Average throughput for each UE
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

            classify_state(ue, ueDelay, state); // Chuyen du lieu tu ue thanh state
            int state_idx = state_to_index(state); // Chuyen state thanh chi so
            int action_idx = select_action_epsilon_greedy(state_idx, EPSILON, QTable); // Chon hanh dong theo epsilon-greed
            schedule_by_action(ue, action_idx, response_data, ueDelay); // Chon ue theo action, update delay
            // Update delay, tbsize_allocated
            for (int i = 0; i < NUM_UE; i++) {
                if (response_data[i].tb_size > 0) {
                    ueDelay[i] = 0; // Reset delay if TB size is allocated
                    ueTBSize_allocated[i] += response_data[i].tb_size; // Update allocated TB size
                } else {
                    ueDelay[i] += 1; // Increase delay if no TB size allocated
                }
            
            // Cap nhat response_data
            reward->throughput = calculate_throughput(response_data, NUM_UE);
            reward->fairness = calculate_fairness(ueTBSize_allocated);
            reward->reward = calculate_reward(reward->throughput, reward->fairness)
            classify_state(ue, delay, state_next); // Chuyen du lieu tu ue thanh state
            int state_next_idx = state_to_index(state_next); // Chuyen next_state thanh chi s
            update_q_table(state_idx, action_idx, reward, state_next_idx, ALPHA, GAMMA, QTable); // Cap nhat bang Q
            
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
    free(state);
    state = NULL;
    free(state_next);
    state_next = NULL;
    free(reward);
    reward = NULL;
    free(QTable);
    QTable = NULL;

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


void classify_state(UEData *ue, int *delay, State *state) {
    /*
        Function to classify the state based on UE data and delay.
        
        Parameters:
            ue: Pointer to the UE data.
            delay: Pointer to the delay array.
            state: Pointer to the State structure to be updated.
        
        Returns:
            None
    */
    // Reset state
    memset(state->cqi_group, 0, sizeof(state->cqi_group));
    memset(state->bsr_group, 0, sizeof(state->bsr_group));
    memset(state->delay_group, 0, sizeof(state->delay_group));

    // Classify CQI, BSR, delay into groups
    for (int i = 0; i < NUM_UE; i++) {
        int cqi_class = (ue[i].cqi <= 5) ? 0 :
                        (ue[i].cqi <= 10) ? 1 : 2;
        state->cqi_group[cqi_class]++;

        // BSR gom thành 2 mức: 0 hoặc 1
        int bsr_class = (ue[i].bsr > 0) ? 1 : 0; 
        state->bsr_group[bsr_class]++;

        // Delay gom thành 2 mức: <=5 → 0, >5 → 1
        int delay_class = (delay[i] <= 15) ? 0 : 
                          (delay[i] <= 30) ? 1 : 2;
        state->delay_group[delay_class]++;
    }
}

int combination(int n, int k) {
    if (k > n) return 0;
    if (k == 0 || k == n) return 1;
    int res = 1;
    for (int i = 1; i <= k; i++) {
        res = res * (n - i + 1) / i;
    }
    return res;
}

int group3_rank(int group[3]) {
    int rank = 0;
    int remain = 12;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < group[i]; j++) {
            rank += combination(remain - j + (2 - i), 2 - i);
        }
        remain -= group[i];
    }
    return rank; // từ 0 đến 90
}

// state index from 0 to 107652
int state_to_index(State *s) {
    int cqi_idx = group3_rank(s->cqi_group);     // [0..90]
    int bsr_idx = s->bsr_group[1];               // [0..12]
    int delay_idx = group3_rank(s->delay_group); // [0..90]

    return (cqi_idx * 13 + bsr_idx) * 91 + delay_idx;
} 

int select_action_epsilon_greedy(int state_idx, double epsilon, float Q[][ACTION]) {
    if ((double)rand() / RAND_MAX < epsilon) {
        return rand() % ACTION;
    }
    int best_a = 0;
    double max_q = Q[state_idx][0];
    for (int a = 1; a < ACTION; a++) {
        if (Q[state_idx][a] > max_q) {
            max_q = Q[state_idx][a];
            best_a = a;
        }
    }
    return best_a;
}

void schedule_by_action(UEData *ue, int action_idx, SchedulerResponse *response, int *delay) {
    int scheduled = 0;
    int rb_per_ue = NUM_RB / MAX_UE_PER_TTI;
    int scheduled_ue[NUM_UE] = {0}; // Đánh dấu các UE đã được lập lịch

    // Khởi tạo kết quả trả về
    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue[i].id;
        response[i].tb_size = 0;
    }

    while (scheduled < MAX_UE_PER_TTI) {
        int selected_idx = -1;
        int max_metric = -1;

        for (int i = 0; i < NUM_UE; i++) {
            if (scheduled_ue[i] || ue[i].bsr <= 0)
                continue;

            int metric = -1;

            if (action_idx == 1) {
                // Ưu tiên CQI cao nhất
                metric = ue[i].cqi;
            } else if (action_idx == 0) {
                // Ưu tiên delay lớn nhất
                metric = delay[i];
            }

            if (metric > max_metric) {
                max_metric = metric;
                selected_idx = i;
            }
        }

        if (selected_idx == -1) break; // Không còn UE phù hợp

        int mcs = cqi_to_mcs(ue[selected_idx].cqi);
        if (mcs < 0 || mcs >= MAX_MCS_INDEX) {
            scheduled_ue[selected_idx] = 1;
            continue;
        }

        int tb_size = TBS[mcs][rb_per_ue - 1];
        if (tb_size <= 0) {
            scheduled_ue[selected_idx] = 1;
            continue;
        }

        response[selected_idx].tb_size = tb_size;
        ue[selected_idx].bsr -= tb_size;
        if (ue[selected_idx].bsr < 0) ue[selected_idx].bsr = 0;

        scheduled_ue[selected_idx] = 1;
        scheduled++;
    }
}

// calculate throughput function
double calculate_throughput(SchedulerResponse *response, int num_ue) {
    /*
        Function to calculate the throughput based on the UE data.
        
        Parameters:
            ue: Pointer to the UE data.
            num_ue: Number of UEs.
        
        Returns:
            Total throughput as a double value.
    */
    double total_throughput = 0.0;
    for (int i = 0; i < num_ue; i++) {
        if (response[i].tb_size > 0) {
            total_throughput += response[i].tb_size; // Assuming tb_size is in bits
        }
    }
    return total_throughput;
}


double calculate_fairness(int ue_tbsize_allocated[NUM_UE]) {    
    /*
        Function to calculate the fairness index based on the allocated TB sizes.
        
        Parameters:
            ue_tbsize_allocated: Array containing the allocated TB sizes for each UE.
        
        Returns:
            Fairness index as a double value.
    */
    double sum = 0, sum_sq = 0;
    for (int i = 0; i < NUM_UE; i++) {
        sum += ue_tbsize_allocated[i];         // tổng RB
        sum_sq += pow(ue_tbsize_allocated[i],2);    // tổng bình phương
    }
    return (pow(sum,2)) / (NUM_UE * sum_sq);
}

double calculate_reward(double throughput, double fairness) {
    /*
        Function to calculate the reward based on throughput and fairness.
        
        Parameters:
            throughput: Total throughput.
            fairness: Fairness index.
        
        Returns:
            Calculated reward as a double value.
    */
    return 0.5 * throughput + 0.5 * fairness; // Reward is the product of throughput and fairness
}


void update_q_table(int state_idx, int action_idx, Reward *reward, int next_state_idx, double alpha, double gamma, float (*Q_table)[ACTION]) {
    /*
        Function to update the Q-table based on the current state, action, reward, and next state.
        
        Parameters:
            state: Pointer to the current state.
            action: Pointer to the action taken.
            reward: Pointer to the reward received.
            next_state: Pointer to the next state after taking the action.
            alpha: Learning rate.
            gamma: Discount factor.
        
        Returns:
            None
    */
    double max_q_next = Q_table[next_state_idx][0];
    for (int i = 1; i < ACTION; i++) {
        if (Q_table[next_state_idx][i] > max_q_next)
            max_q_next = Q_table[next_state_idx][i];
    }
    Q_table[state_idx][action_idx] += alpha * (reward->reward + gamma * max_q_next - Q_table[state_idx][action_idx]);
}

