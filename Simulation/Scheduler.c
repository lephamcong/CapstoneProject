/**
 * @file Scheduler.c
 * @brief This file simulate the Scheduler in MAC layer of 5G NR
 * 
 * The main functionalities include:
 * - Scheduling resource (Transport Block Size) (bytes) for each UE
 * - Receiving and sending data from/to UE module
 * - Save scheduling data to Storage
 * 
 * Functions:
 * - 
 * - 
 * 
 * The main function performs the following steps: 
 * - Receive UE data from UE module
 * - Scheduling resource for each UE
 * - Response scheduling resource to UE module
 * - Repeats the process for multiple interations
 * - Log results to Storage 
 * */ 
#include "define.h"

int TBSArray[MAX_MCS_INDEX][NUM_RB];

int main() {
    int check_value;
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

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
    // Tạo socket UDP
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        LOG_ERROR("Cannot create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket vào địa chỉ và cổng
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        LOG_ERROR("Bind failed");
        close(sock);
        return 1;
    }

    printf("Scheduler is listening on port %d...\n", SERVER_PORT);

    while (1) {
        recvfrom(sock, ue, sizeof(UEData) * NUM_UE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (check_value < 0) {
            LOG_ERROR("Cannot recvfrom UE");
        }

        for (int i = 0; i < NUM_UE; i++) {
            printf("Received UE%d: TTI=%d, MCS=%d, BSR=%d\n", ue[i].id, ue[i].tti, ue[i].mcs, ue[i].bsr);
        }
        printf("--------------------------\n");

        // Lập lịch (giả lập) dựa trên MCS và BSR
        for (int i = 0; i < NUM_UE; i++) {
            response[i].TBSize = 100;
        }

        sendto(sock, response, NUM_UE*sizeof(SchedulerResponse), 0, (struct sockaddr *)&client_addr, addr_len);
        if (check_value < 0) {
            LOG_ERROR("Cannot sendto UE");
        }
    }

    close(sock);
    return 0;
}
