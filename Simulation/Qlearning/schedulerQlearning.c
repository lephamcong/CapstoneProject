#include "../define.h"
#include "DQL.c"

#define TTI_DURATION_NS 1000000L // 1ms = 1,000,000 nanoseconds


int TBS[MAX_MCS_INDEX][NUM_RB];

int cqi_to_mcs(int cqi);    // Mapping CQI to MCS index
void TBS_Table();           // Load TBS table from CSV file
void log_tbsize(FILE *log_file, int tti, SchedulerResponse *response, int num_ue);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        LOG_ERROR("Missing scenario file.");
        return 1;
    }
    const char *result_pathfile = argv[1];

    // Initialize replay buffer
    ReplayBuffer rb;
    init_replay_buffer(&rb, REPLAY_BUFFER_SIZE);

    // variables for algorithm
    float epsilon = 1.0f;
    const float epsilon_min = 0.05f;
    const float decay_rate = 0.0005f;
    int delay[NUM_UE] = {0};
    int cumulative_tput[NUM_UE] = {0}; // để tính Jain index

    int tti_now = 0, tti_last = 0;

    // Initialize networks, load model if PROCESS is 1. Copy the network to target_net
    NeuralNet net, target_net;
    if (PROCESS == 1){
        if (!load_model("./model/dqn_model.bin", &net)) {
            LOG_INFO("Cannot find model, initialze\n");
            init_net(&net);
        }
        epsilon = 0.25f; 
    }
    copy_network(&target_net, &net);

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

    // Open log file
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

    // Open reward log file
    FILE *reward_log = NULL;
    if (PROCESS == 0) {
        reward_log = fopen("./result/reward_log_prev.csv", "w");
    } else {
        reward_log = fopen("./result/reward_log.csv", "w");
    }
    if (!reward_log) {
        LOG_ERROR("Không thể mở file reward_log.csv");
        exit(EXIT_FAILURE);
    }
    fprintf(reward_log, "TTI,Reward,Epsilon\n");


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
                if (sem_post(sem_scheduler_recv) == -1) {
                    LOG_ERROR("Failed to post semaphore for UE data");
                    return 1;
                } else {
                    LOG_OK("Semaphore for UE data posted successfully");
                }
            }

// ------------------------- Q Learning ---------------------------------
            // Normalize state
            float state[INPUT_SIZE];
            for (int i = 0; i < NUM_UE; i++) {
                state[i*3 + 0] = ue[i].cqi / 15.0f;                         // Max CQI = 15
                state[i*3 + 1] = fminf(ue[i].bsr, 18960) / 18960.0f;        // Max BSR = 18960 bits (for mcs = 27 and 25 RBs)
                state[i*3 + 2] = fminf(delay[i], 20.0f) / 20.0f;            // Set delay max = 20ms
            }

            // Forward network to get Q values
            float hidden[HIDDEN_SIZE], q_values[OUTPUT_SIZE];
            forward(&net, state, hidden, q_values);

            // Choose action using epsilon-greedy policy
            int action_idx = epsilon_greedy_action(q_values, epsilon, ue);
            int selected_ue[4];
            index_to_ue_combination(action_idx, selected_ue);

            // Update response_data with selected UEs
            for (int i = 0; i < NUM_UE; i++) {
                response_data[i].id = ue[i].id;
                response_data[i].tb_size = 0;
            }
            for (int i = 0; i < 4; i++) {
                int idx = selected_ue[i];
                int mcs = cqi_to_mcs(ue[idx].cqi);
                response_data[idx].tb_size = TBS[mcs][NUM_RB_PER_UE - 1];
            }
            // Update BSR and delay
            for (int i = 0; i < NUM_UE; i++) {
                if (response_data[i].tb_size > 0) {
                    delay[i] = 0; 
                    ue[i].bsr -= response_data[i].tb_size;
                    if (ue[i].bsr < 0) {
                        ue[i].bsr = 0;
                    }
                } else {
                    delay[i] += SUBFRAME_DURATION; 
                }
            }

            // Calculate reward
            float reward = compute_reward(response_data, cumulative_tput, NUM_UE, delay);

            // Normalize next state
            float next_state[INPUT_SIZE];
            for (int i = 0; i < NUM_UE; i++) {
                next_state[i*3 + 0] = ue[i].cqi / 15.0f;                     
                next_state[i*3 + 1] = fminf(ue[i].bsr, 18960) / 18960.0f; 
                next_state[i*3 + 2] = fminf(delay[i], 20.0f) / 20.0f; 
            }

            // Create transition and add to replay buffer
            Transition t;
            memcpy(t.state, state, sizeof(float) * INPUT_SIZE);
            t.action_index = action_idx;
            t.reward = reward;
            memcpy(t.next_state, next_state, sizeof(float) * INPUT_SIZE);
            t.done = (tti_now == NUM_TTI);
            add_transition(&rb, t);

            // Train the network
            Transition batch[BATCH_SIZE];
            if (sample_batch(&rb, batch, BATCH_SIZE)) {
                train_once(&net, &target_net, batch, BATCH_SIZE, 0.99f); // γ = 0.99
                if (tti_now % TARGET_UPDATE_INTERVAL == 0) {
                    copy_network(&target_net, &net);
                }
            }

            // 9. Reduce epsilon
            if (PROCESS == 0){
                if (tti_now < 3000) {
                    epsilon = 1.0f;
                } else if (tti_now < 15000) {
                    float progress = (float)(tti_now - 3000) / 12000.0f;
                    epsilon = 1.0f - 0.75f * progress;  // reduce 1.0 → 0.25
                } else {
                    epsilon = 0.25f;  // keep epsilon at 0.25 after 15000 TTIs
                }
            }

            // Log the reward and epsilon
            fprintf(reward_log, "%d,%.4f,%.4f\n", tti_now, reward, epsilon);
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
            printf("TTI: %d\n", tti_now);
        }
    }

    // log file replaybuffer
    FILE *replay_buffer_file = fopen("./result/replay_buffer.csv", "w");
    if (replay_buffer_file == NULL) {
        LOG_ERROR("Error opening replay buffer log file");
        return 1;
    } else {
        LOG_OK("Replay buffer log file opened successfully");
    }
    fprintf(replay_buffer_file, "State,Action,Reward,Next_State,Done\n");
    for (int i = 0; i < rb.size; i++) {
        Transition t = rb.buffer[i];
        fprintf(replay_buffer_file, 
            "%s,%d,%.4f,%s,%d\n", 
            state_to_string(t.state), 
            t.action_index, 
            t.reward, 
            state_to_string(t.next_state), 
            t.done
        );
    }

    // Save the model if PROCESS is train
    if (PROCESS == 0) {
        save_model("./model/dqn_model.bin", &net);
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
