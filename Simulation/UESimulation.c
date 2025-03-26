/**
 * @file UESimulation.c
 * @brief This file contains functions to simulate User Equipment (UE) behavior in 5G NR.
 *
 * The main functionalities include:
 * - Initializing UE data with random values.
 * - Updating UE data based on Transport Block (TB) size.
 * - Sending and receiving data to/from MAC Scheduler.
 *
 * Functions:
 * - void init_data(UEData *ue, int id): Initializes UE data with random values.
 * - void update_data(UEData* ue, int TBSize): Updates UE data based on TB size.   
 *  
 * The main function performs the following steps:
 * - Initializes UE data for all UEs.
 * - Sends data to MAC Scheduler.
 * - Receives data from MAC Scheduler.
 * - Updates UE data based on TB size.
 * - Repeats the process for multiple iterations.
 */

#include "define.h"

void init_data(UEData *ue, int id);
void update_data(UEData* ue, int TBSize);

int main() {
    int check_value;
    char buffer[sizeof(UEData)*NUM_UE + sizeof(TransInfo)];

    int sock;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        LOG_ERROR("Cannot create socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // srand(time(NULL));

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

    TransInfo *transport_infomation = (TransInfo*) malloc(NUM_UE*sizeof(TransInfo));
    if (transport_infomation == NULL) {
        LOG_ERROR("Memory allocation failed\n");
        return 1;
    }    

    for (int i = 0; i < NUM_UE; i++) {
        init_data(&ue[i],i+1);
    }

    for (int tti = 1; tti <= NUM_TTI; tti++) {
        transport_infomation->tti = tti;
        memcpy(buffer, transport_infomation, sizeof(TransInfo));
        memcpy(buffer + sizeof(TransInfo), ue, sizeof(UEData)*NUM_UE);

        check_value = sendto(sock, buffer, sizeof(UEData) * NUM_UE, 0, (struct sockaddr *)&server_addr, addr_len);
        if (check_value < 0) {
            LOG_ERROR("Cannot sendto Scheduler");
            close(sock);
            return 1;
        }
        else printf("UE sent: TTI=%d \n",tti);

        check_value = recvfrom(sock, response, NUM_UE*sizeof(SchedulerResponse), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (check_value < 0) {
            LOG_ERROR("Cannot recvfrom Scheduler");
            close(sock);
            return 1;
        }
        for (int i = 0; i < NUM_UE; i++) {
            update_data(&ue[i], response[i].TBSize);
        }

        // usleep(1000); // Giả lập thời gian mỗi TTI = 1ms
    }

    close(sock);
    return 0;
}


void init_data(UEData *ue, int id){
    ue->id = id;
    ue->bsr = rand();
    if (id>0  && id<=4) {
        ue->mcs = rand()%9 + 19;
    }
    else if (id>4 && id<=8){
        ue->mcs = rand()%9 + 10;
    }
    else if (id>8 && id<=12){
        ue->mcs = rand()%10;
    }
    else{
        printf("Invalid UE ID\n");
    }
    ue->bsr = rand()%10000;
}

void update_data(UEData* ue, int TBSize){
    ue->bsr = (ue->bsr > TBSize ? ue->bsr - TBSize : 0);
    ue->mcs = ue->mcs + rand()%3 - 1;
    if (ue->mcs < 0) {
        ue->mcs = 0;
    } else if (ue->mcs > 27) {
        ue->mcs = 27;
    }
}