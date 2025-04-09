
 #include "define.h"

 void init_data(UEData *ue, int id);
 void update_data(UEData* ue, int TBSize);
 
 int main() {
     char buffer[sizeof(UEData) * NUM_UE];
 
     int sock;
     struct sockaddr_un server_addr, client_addr;
 
     /*create socket*/
     sock = socket(AF_UNIX, SOCK_STREAM, 0);
     if (sock == -1) {
         LOG_ERROR("Cannot create socket");
         return 1;
     }
 
     /*link socket path to client*/
     unlink(UE_SIMULATION);
     memset(&client_addr, 0, sizeof(client_addr));
     client_addr.sun_family = AF_UNIX;
     strcpy(client_addr.sun_path, UE_SIMULATION);
 
     if(connect(sock, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
         LOG_ERROR("Cannot connect to MAC Scheduler");
         close(sock);
         return 1;
     }

     /*link socket path to server*/
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sun_family = AF_UNIX;
     strcpy(server_addr.sun_path, UE_SIMULATION);
     
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
         init_data(&ue[i],i+1);
     }
 
     int TTI = 1;
     while(TTI <= TOTAl_TTI){
         memcpy(buffer, ue, sizeof(UEData)*NUM_UE);
 
        if (write(sock, buffer, sizeof(UEData)*NUM_UE) == -1) {
            LOG_ERROR("Can't send data to MAC Scheduler");
            close(sock);
            return 1;
        }else {
            printf("UE sent: TTI=%d \n",TTI);
        }
         socklen_t server_addr_len = sizeof(server_addr);
         if (read(sock, buffer, sizeof(SchedulerResponse)*NUM_UE) == -1) {
             LOG_ERROR("Can't receive data from MAC Scheduler");
             close(sock);
             return 1;
         } else {
             memcpy(response, buffer, sizeof(SchedulerResponse)*NUM_UE);
         }
         for(int i = 0; i < NUM_UE; i++){
             printf("UE%d: TTI=%d, TBSize=%d\n", i+1, TTI, response[i].TBSize);
             update_data(&ue[i], response[i].TBSize);
         }
            TTI += 1;
         printf("--------------------------\n");
     }
 
 
     close(sock);
     return 0;
 }
 
 void init_data(UEData *ue, int id){
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
 