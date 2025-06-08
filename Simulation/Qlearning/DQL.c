#include "../define.h"

// define neural network parameters
#define INPUT_SIZE 36
#define HIDDEN_SIZE 64
#define OUTPUT_SIZE 495
#define LEARNING_RATE 0.001

// Proocess = 1: test, Process = 2: train
#if PROCESS == 1
#define REPLAY_BUFFER_SIZE 10000
#else 
#define REPLAY_BUFFER_SIZE 20000
#endif
#define BATCH_SIZE 64

#define TARGET_UPDATE_INTERVAL 1300
#define CLIP_VALUE 1.0f

// define structures
typedef struct {
    float W1[HIDDEN_SIZE][INPUT_SIZE];      // Weights for input to hidden layer
    float b1[HIDDEN_SIZE];                  // Bias for hidden layer    
    float W2[OUTPUT_SIZE][HIDDEN_SIZE];     // Weights for hidden to output layer
    float b2[OUTPUT_SIZE];                  // Bias for output layer
} NeuralNet;

typedef struct {
    float state[INPUT_SIZE];       // Current state of UEs (36 features)
    int action_index;              // Current action index (0-494)
    float reward;                  // Reward received after taking action
    float next_state[INPUT_SIZE];  // Next state of UEs after action
    int done;                      // 1 if episode is done, 0 otherwise
} Transition;

typedef struct {
    Transition *buffer;             
    int size;
    int next_idx;
} ReplayBuffer;

void init_net(NeuralNet *net);
void forward(NeuralNet *net, float *input, float *hidden, float *output);

// neutral
float relu(float x) { return x > 0 ? x : 0; }
float relu_derivative(float x) { return x > 0 ? 1 : 0; }

void init_net(NeuralNet *net) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        for (int j = 0; j < INPUT_SIZE; j++) {
            net->W1[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
        net->b1[i] = 0;
    }
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            net->W2[i][j] = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
        }
        net->b2[i] = 0;
    }
}

void forward(NeuralNet *net, float *input, float *hidden, float *output) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        hidden[i] = net->b1[i];
        for (int j = 0; j < INPUT_SIZE; j++) {
            hidden[i] += net->W1[i][j] * input[j];
        }
        hidden[i] = relu(hidden[i]);
    }
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        output[i] = net->b2[i];
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            output[i] += net->W2[i][j] * hidden[j];
        }
    }
}

// action
void index_to_ue_combination(int idx, int *out) {
    int cnt = 0;
    for (int a = 0; a < 9; a++) {
        for (int b = a + 1; b < 10; b++) {
            for (int c = b + 1; c < 11; c++) {
                for (int d = c + 1; d < 12; d++) {
                    if (cnt == idx) {
                        out[0] = a; out[1] = b; out[2] = c; out[3] = d;
                        return;
                    }
                    cnt++;
                }
            }
        }
    }
}

extern int TBS[MAX_MCS_INDEX][NUM_RB];
extern int cqi_to_mcs(int cqi);

// relay buffer
void init_replay_buffer(ReplayBuffer *rb, int capacity) {
    rb->buffer = (Transition*) malloc(sizeof(Transition) * capacity);
    if (!rb->buffer) {
        LOG_ERROR("Không thể cấp phát replay buffer!");
        exit(1);
    }
    rb->size = 0;
    rb->next_idx = 0;
}

void add_transition(ReplayBuffer *rb, Transition t) {
    rb->buffer[rb->next_idx] = t;
    rb->next_idx = (rb->next_idx + 1) % REPLAY_BUFFER_SIZE;
    if (rb->size < REPLAY_BUFFER_SIZE)
        rb->size++;
}

int sample_batch(ReplayBuffer *rb, Transition *out_batch, int batch_size) {
    if (rb->size < batch_size) return 0;
    for (int i = 0; i < batch_size; i++) {
        int idx = rand() % rb->size;
        out_batch[i] = rb->buffer[idx];
    }
    return 1;
}

// Train once
void train_once(NeuralNet *net, NeuralNet *target_net, Transition *batch, int batch_size, float gamma) {
    // Initialize gradients
    float dW1_sum[HIDDEN_SIZE][INPUT_SIZE] = {0};
    float db1_sum[HIDDEN_SIZE] = {0};
    float dW2_sum[OUTPUT_SIZE][HIDDEN_SIZE] = {0};
    float db2_sum[OUTPUT_SIZE] = {0};

    float total_loss = 0.0f;

    for (int b = 0; b < batch_size; b++) {
        Transition *t = &batch[b];

        // Foward network to get Q(current_state)
        float hidden1[HIDDEN_SIZE], output1[OUTPUT_SIZE];
        forward(net, t->state, hidden1, output1);

        // Forward network to get Q(next_state)
        float hidden2[HIDDEN_SIZE], output2[OUTPUT_SIZE];
        forward(target_net, t->next_state, hidden2, output2);

        // Calculate Q max
        float max_q_next = output2[0];
        for (int i = 1; i < OUTPUT_SIZE; i++) {
            if (output2[i] > max_q_next) max_q_next = output2[i];
        }
        // Calculate Q-target using Bellman equation
        float q_target = t->reward + (t->done ? 0.0f : gamma * max_q_next);
        float q_current = output1[t->action_index];
        float loss_grad = 2.0f * (q_current - q_target); // dLoss/dQ
        
        if (loss_grad > CLIP_VALUE) loss_grad = CLIP_VALUE;
        if (loss_grad < -CLIP_VALUE) loss_grad = -CLIP_VALUE;

        total_loss += (q_current - q_target) * (q_current - q_target);

        // Backprop: W2, b2
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            dW2_sum[t->action_index][j] += loss_grad * hidden1[j];
        }
        db2_sum[t->action_index] += loss_grad;

        // Backprop: W1, b1
        float delta1[HIDDEN_SIZE] = {0};
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            delta1[j] = loss_grad * net->W2[t->action_index][j] * relu_derivative(hidden1[j]);
        }

        for (int i = 0; i < HIDDEN_SIZE; i++) {
            for (int j = 0; j < INPUT_SIZE; j++) {
                dW1_sum[i][j] += delta1[i] * t->state[j];
            }
            db1_sum[i] += delta1[i];
        }
    }

    float clip_value = 1.0f;
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            if (dW2_sum[i][j] > clip_value) dW2_sum[i][j] = clip_value;
            if (dW2_sum[i][j] < -clip_value) dW2_sum[i][j] = -clip_value;
        }
        if (db2_sum[i] > clip_value) db2_sum[i] = clip_value;
        if (db2_sum[i] < -clip_value) db2_sum[i] = -clip_value;
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        for (int j = 0; j < INPUT_SIZE; j++) {
            if (dW1_sum[i][j] > clip_value) dW1_sum[i][j] = clip_value;
            if (dW1_sum[i][j] < -clip_value) dW1_sum[i][j] = -clip_value;
        }
        if (db1_sum[i] > clip_value) db1_sum[i] = clip_value;
        if (db1_sum[i] < -clip_value) db1_sum[i] = -clip_value;
    }

    // Update weights and biases
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            net->W2[i][j] -= LEARNING_RATE * dW2_sum[i][j] / batch_size;
        }
        net->b2[i] -= LEARNING_RATE * db2_sum[i] / batch_size;
    }

    for (int i = 0; i < HIDDEN_SIZE; i++) {
        for (int j = 0; j < INPUT_SIZE; j++) {
            net->W1[i][j] -= LEARNING_RATE * dW1_sum[i][j] / batch_size;
        }
        net->b1[i] -= LEARNING_RATE * db1_sum[i] / batch_size;
    }
}


float compute_jain_index(int *throughput, int num_ue) {
    int sum = 0;
    int sum_sq = 0;
    for (int i = 0; i < num_ue; i++) {
        sum += throughput[i];
        sum_sq += throughput[i] * throughput[i];
    }
    if (sum == 0) return 0.0f;
    return ((float)(sum * sum)) / (num_ue * (float)sum_sq + 1e-6f); // tránh chia 0
}

float compute_reward(SchedulerResponse *response, int *prev_total_throughput, int num_ue, int* delay) {
    int tput[NUM_UE] = {0};
    int sum_tput = 0;

    for (int i = 0; i < num_ue; i++) {
        tput[i] = response[i].tb_size;
        prev_total_throughput[i] += tput[i];
        sum_tput += tput[i];
    }

    int delay_sum = 0;
    for (int i = 0; i < num_ue; i++) {
        delay_sum += delay[i];
    }
    float avg_delay = (float)delay_sum / num_ue;
    if (avg_delay > 20.0f) {
        avg_delay = 10.0f; // Limiting delay to a maximum of 10 seconds
    }

    float fairness = compute_jain_index(prev_total_throughput, num_ue);
    float reward = 0.3 * (float) sum_tput / 18960 / 4 + 0.6 * fairness -  0.05 * avg_delay;
    return reward;
}


int epsilon_greedy_action(float *Q_value, float epsilon, UEData *ue_data) {
    int valid_actions[OUTPUT_SIZE];
    int num_valid = 0;

    // Make valid actions list    
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        int ue_sel[4];
        index_to_ue_combination(i, ue_sel);
        int valid = 1;
        for (int j = 0; j < 4; j++) {
            if (ue_data[ue_sel[j]].bsr == 0) {
                valid = 0;
                break;
            }
        }
        if (valid) {
            valid_actions[num_valid++] = i;
        }
    }

    // If no valid actions, return a random action
    if (num_valid == 0) {
        return rand() % OUTPUT_SIZE; // fallback bất đắc dĩ
    }

    float r = (float)rand() / RAND_MAX;

    // Exploration
    if (r < epsilon) {
        return valid_actions[rand() % num_valid];
    }

    // Exploitation
    int best_idx = valid_actions[0];
    float max_q = Q_value[best_idx];
    for (int i = 1; i < num_valid; i++) {
        int idx = valid_actions[i];
        if (Q_value[idx] > max_q) {
            max_q = Q_value[idx];
            best_idx = idx;
        }
    }

    return best_idx;
}

void copy_network(NeuralNet *dst, NeuralNet *src) {
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        for (int j = 0; j < INPUT_SIZE; j++) {
            dst->W1[i][j] = src->W1[i][j];
        }
        dst->b1[i] = src->b1[i];
    }
    for (int i = 0; i < OUTPUT_SIZE; i++) {
        for (int j = 0; j < HIDDEN_SIZE; j++) {
            dst->W2[i][j] = src->W2[i][j];
        }
        dst->b2[i] = src->b2[i];
    }
}

// Convert state to string for logging
const char* state_to_string(float *state) {
    static char str[256];
    str[0] = '\0';
    for (int i = 0; i < INPUT_SIZE; i++) {
        char temp[16];
        snprintf(temp, sizeof(temp), "%.4f,", state[i]);
        strcat(str, temp);
    }
    str[strlen(str) - 1] = '\0'; // remove last comma
    return str;
}

// Save and load model functions
void save_model(const char *filename, NeuralNet *net) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        LOG_ERROR("Cannot open file to save model");
        return;
    }
    fwrite(net, sizeof(NeuralNet), 1, fp);
    fclose(fp);
}

int load_model(const char *filename, NeuralNet *net) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Cannot open file to load model");
        return 0;
    }
    fread(net, sizeof(NeuralNet), 1, fp);
    fclose(fp);
    return 1;
}