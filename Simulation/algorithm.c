#include "define.h"

UEData ue[MAX_UE]; // Array to store UE data
SchedulerResponse response[MAX_UE]; // Array to store Scheduler response

int cqi_to_mcs(int cqi);    // Mapping CQI to MCS index
void TBS_Table();           // Load TBS table from CSV file

void get_data_from_file(const char *cqi_filename, const char *bsr_filename, int tti, UEData *ue);
void update_data(UEData* ue, int TBSize);
void RoundRobin(UEData* ue_data, SchedulerResponse *response);
void MaxCQI(UEData *ue_data, SchedulerResponse *response, int *tti_since_last_sched);
void ProportionalFair(UEData *ue_data, SchedulerResponse *response,
                      float *avg_throughput, int *tti_since_last_sched);
int TBS[MAX_MCS_INDEX][NUM_RB];

int main() {
    const char *cqi_file = "/home/kaonashi/CapstoneProject/CapstoneProject/Simulation/data/cqi_high_traffic.csv";
    const char *bsr_file = "/home/kaonashi/CapstoneProject/CapstoneProject/Simulation/data/bsr_high_traffic.csv";

    TBS_Table(); 
    float avg_throughput[NUM_UE] = {0};         // Trung bình tốc độ ban đầu
    int tti_since_last_sched[NUM_UE] = {38};     // Số TTI bị bỏ qua ban đầu

    for (int tti = 1; tti <= 40; tti++) {
        printf("\n=== TTI %d ===\n", tti);

        if (tti % 2 == 1) {
            get_data_from_file(cqi_file, bsr_file, tti, ue);
            printf("[Load] Loaded CQI & BSR data at TTI %d:\n", tti);
            for (int i = 0; i < MAX_UE; i++) {
                printf("UE %d: CQI=%d, BSR=%d\n", ue[i].id, ue[i].cqi, ue[i].bsr);
            }
        }
        ProportionalFair(ue, response, avg_throughput, tti_since_last_sched);

        // MaxCQI(ue, response,tti_since_last_sched);
        printf("[Scheduling] Result at TTI %d:\n", tti);
        for (int i = 0; i < MAX_UE; i++) {
            printf("UE %d: TBSize=%d, BSR còn lại=%d\n", response[i].id, response[i].tb_size, ue[i].bsr);
            
        }
    }

    return 0;
}


void get_data_from_file(const char *cqi_filename, const char *bsr_filename, int tti, UEData *ue){
    /*
        Function to read data from files and update UE data.
            UEData structure contains:
                - id: UE ID
                - cqi: Channel Quality Indicator
                - bsr: Buffer Status Report
        Parameters:
            cqi_filename: Name of the file containing CQI data.
            bsr_filename: Name of the file containing BSR data.
            tti: TTI value to read data for.
            ue: Pointer to UEData structure to be updated.
    */
    FILE *cqi_file = fopen(cqi_filename, "r");
    FILE *bsr_file = fopen(bsr_filename, "r");
    if (cqi_file == NULL || bsr_file == NULL) {
        LOG_ERROR("Error opening file for reading");
        return;
    }
    char line1[MAX_LINE_LENGTH],line2[MAX_LINE_LENGTH];
    int current_tti  = 1;
    int count_UE;

    while (fgets(line1, sizeof(line1), cqi_file) && fgets(line2, sizeof(line2), bsr_file)) {
        if (current_tti == tti + 1) {
            char *token1 = strtok(line1, ",");
            count_UE = 0;
            while (token1 != NULL) {
                ue[count_UE].id = count_UE + 1; 
                ue[count_UE].cqi = atoi(token1); // Convert string to integer
                count_UE++;
                token1 = strtok(NULL, ",");
            }
            char *token2 = strtok(line2, ",");
            count_UE = 0;
            while (token2 != NULL) {
                ue[count_UE].bsr += atoi(token2); // Convert string to integer
                count_UE++;
                token2 = strtok(NULL, ",");
            }
        }
        current_tti++;
    }
    // for (int i = 0; i < MAX_UE; i++) {
    //     printf("[In get data function] UE %d: CQI=%d, BSR=%d\n", ue[i].id, ue[i].cqi, ue[i].bsr);
    // }
    fclose(cqi_file);
    fclose(bsr_file);
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
    // ue->cqi = ue->cqi + rand()%3 - 1;
    // if (ue->cqi < 0) {
        // ue->cqi = 0;
    // } else if (ue->cqi > 27) {
        // ue->cqi = 27;
    // }
}

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
    FILE *file = fopen("/home/kaonashi/CapstoneProject/CapstoneProject/TBS/TBSArray.csv", "r");
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


#define MAX_TTI_WITHOUT_SCHED 40

void ProportionalFair(UEData *ue_data, SchedulerResponse *response,
                      float *avg_throughput, int *tti_since_last_sched) {
    int scheduled = 0;
    int rb_per_ue = NUM_RB / MAX_UE_PER_TTI;

    int scheduled_ue[NUM_UE] = {0};  // Đánh dấu ai đã được lập lịch trong TTI này

    // Reset phản hồi
    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 0;
    }

    while (scheduled < MAX_UE_PER_TTI) {
        float max_metric = -1.0;
        int selected = -1;

        for (int i = 0; i < NUM_UE; i++) {
            if (scheduled_ue[i] || ue_data[i].bsr <= 0 || ue_data[i].cqi <= 0)
                continue;

            int mcs = cqi_to_mcs(ue_data[i].cqi);
            if (mcs < 0 || mcs >= MAX_MCS_INDEX) continue;

            int tb_size = TBS[mcs][rb_per_ue - 1];
            if (tb_size <= 0) continue;

            float inst_throughput = (float)tb_size;
            float metric;

            if (tti_since_last_sched[i] >= MAX_TTI_WITHOUT_SCHED) {
                metric = 1e9;  // Ưu tiên tuyệt đối
            } else if (avg_throughput[i] > 0.0) {
                metric = inst_throughput / avg_throughput[i];
            } else {
                metric = inst_throughput;
            }

            if (metric > max_metric) {
                max_metric = metric;
                selected = i;
            }
        }

        if (selected == -1) break;

        int mcs = cqi_to_mcs(ue_data[selected].cqi);
        int tb_size = TBS[mcs][rb_per_ue - 1];

        response[selected].tb_size = tb_size;
        ue_data[selected].bsr -= tb_size;
        if (ue_data[selected].bsr < 0) ue_data[selected].bsr = 0;

        // Cập nhật throughput trung bình cho UE được lập lịch
        avg_throughput[selected] = (1.0 - ALPHA) * avg_throughput[selected] + ALPHA * tb_size;

        tti_since_last_sched[selected] = 0;  // Reset do được lập lịch
        scheduled_ue[selected] = 1;
        scheduled++;
    }

    for (int i = 0; i < NUM_UE; i++) {
        if (!scheduled_ue[i]) {
            tti_since_last_sched[i]++;
            avg_throughput[i] = (1 - ALPHA) * avg_throughput[i];  // Giảm trung bình theo thời gian
        }
    }

    // print tti_since_last_sched and avg_throughput
    for (int i = 0; i < NUM_UE; i++) {
        printf("UE %d: TSLS=%d, AvgThroughput=%.2f\n", ue_data[i].id, tti_since_last_sched[i], avg_throughput[i]);
    }
}


// Function to implement Round Robin scheduling algorithm
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

        if (cqi <= 0) {
            //printf("[WARN] UE %d có CQI=%d không hợp lệ -> Bỏ qua\n", ue_data[idx].id, cqi);
            continue;
        }

        int mcs = cqi_to_mcs(cqi);
        if (mcs < 0 || mcs >= MAX_MCS_INDEX) {
            // printf("[WARN] UE %d có MCS không hợp lệ (CQI=%d → MCS=%d) -> Bỏ qua\n", ue_data[idx].id, cqi, mcs);
            continue;
        }

        int tb_size = TBS[mcs][rb_per_ue - 1];
        if (tb_size <= 0) {
            //printf("[WARN] TBS không hợp lệ tại MCS=%d, RB=%d -> Bỏ qua UE %d\n", mcs, rb_per_ue - 1, ue_data[idx].id);
            continue;
        }

        if (ue_data[idx].bsr > 0) {
            response[idx].id = ue_data[idx].id;
            response[idx].tb_size = tb_size;

            ue_data[idx].bsr -= tb_size;
            if (ue_data[idx].bsr < 0)
                ue_data[idx].bsr = 0;

            scheduled++;
        } else {
            //printf("[INFO] UE %d có BSR=0 -> Không cấp tài nguyên\n", ue_data[idx].id);
        }
    }
    last_index = (last_index + scheduled) % NUM_UE;
}

// Function to implement Max C/I scheduling algorithm
void MaxCQI(UEData *ue_data, SchedulerResponse *response, int *tti_since_last_sched) {
    int scheduled = 0;
    int rb_per_ue = NUM_RB / MAX_UE_PER_TTI;

    for (int i = 0; i < NUM_UE; i++) {
        response[i].id = ue_data[i].id;
        response[i].tb_size = 0;
    }

    int scheduled_ue[NUM_UE] = {0};

    while (scheduled < MAX_UE_PER_TTI) {
        int max_cqi = -1;
        int max_idx = -1;

        for (int i = 0; i < NUM_UE; i++) {
            if (!scheduled_ue[i] && ue_data[i].bsr > 0 && ue_data[i].cqi > 0) {
                if (ue_data[i].cqi > max_cqi) {
                    max_cqi = ue_data[i].cqi;
                    max_idx = i;
                } else if (ue_data[i].cqi == max_cqi) {
                    // So sánh thời gian chưa được lập lịch
                    if (tti_since_last_sched[i] > tti_since_last_sched[max_idx]) {
                        max_idx = i;
                    } else if (tti_since_last_sched[i] == tti_since_last_sched[max_idx] && i < max_idx) {
                        max_idx = i;
                    }
                }
            }
        }

        if (max_idx == -1) break;  // Không còn UE hợp lệ

        int mcs = cqi_to_mcs(ue_data[max_idx].cqi);
        if (mcs < 0 || mcs >= MAX_MCS_INDEX) {
            printf("[WARN] MCS không hợp lệ cho CQI=%d của UE %d\n", ue_data[max_idx].cqi, ue_data[max_idx].id);
            scheduled_ue[max_idx] = 1;
            continue;
        }

        int tb_size = TBS[mcs][rb_per_ue - 1];
        if (tb_size <= 0) {
            printf("[WARN] TBS lỗi tại MCS=%d\n", mcs);
            scheduled_ue[max_idx] = 1;
            continue;
        }

        response[max_idx].tb_size = tb_size;
        ue_data[max_idx].bsr -= tb_size;
        if (ue_data[max_idx].bsr < 0) ue_data[max_idx].bsr = 0;

        tti_since_last_sched[max_idx] = 0; // Được lập lịch → reset

        scheduled_ue[max_idx] = 1;
        scheduled++;
    }

    // Cập nhật TTI chờ cho UE chưa được lập lịch
    for (int i = 0; i < NUM_UE; i++) {
        if (!scheduled_ue[i])
            tti_since_last_sched[i]++;
    }
}
