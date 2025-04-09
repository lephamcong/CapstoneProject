#include "define.h"
#include "./roundRobin.c"
#include "./MaxC_I.c"
#include "./PF.c"

int TBSArray[MAX_MCS_INDEX][NUM_RB];

int main() {
    char buffer[sizeof(UEData) * NUM_UE];

    int sock;
    struct sockaddr_un server_addr, client_addr;
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

    /*create socket*/
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        LOG_ERROR("Cannot create socket");
        return 1;
    }

    /*link socket path to server*/
    unlink(SCHEDULER);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SCHEDULER);

    
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        LOG_ERROR("Bind failed");
        close(sock);
        return 1;
    } 

    if (listen(sock, 5) == -1) {
        LOG_ERROR("Listen failed");
        close(sock);
        return 1;
    }

    if (accept(sock, (struct sockaddr *)&client_addr, &addr_len) == -1) {
        LOG_ERROR("Accept failed");
        close(sock);
        return 1;
    } else {
        printf("Connected to UE Simulation\n");
    }
    int TTI = 0;
    
    while(TTI<TOTAl_TTI) {
        if (read(sock, buffer, sizeof(UEData) * NUM_UE) == -1) {
            LOG_ERROR("Read failed");
            close(sock);
            return 1;
        } else {
            memcpy(ue, buffer, sizeof(UEData) * NUM_UE);
            TTI += 1;
        }

        for (int i = 0; i < NUM_UE; i++) {
            printf("Received UE%d: TTI=%d, MCS=%d, BSR=%d\n", i+1, TTI, ue[i].mcs, ue[i].bsr);
        }
        printf("--------------------------\n");

        for (int i = 0; i < NUM_UE; i++) {
            response[i].TBSize = 100;
        }
        if (write(sock, response, sizeof(SchedulerResponse) * NUM_UE) == -1) {
            LOG_ERROR("Write failed");
            close(sock);
            return 1;
        } 


    }

    close(sock);
    unlink(SCHEDULER);

    return 0;
}
