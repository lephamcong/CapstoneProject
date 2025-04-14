#include "define.h"

int main(){
    sem_unlink(SEM_CQI);
    sem_unlink(SEM_DCI);
    sem_unlink(SEM_SYNC);
    shm_unlink(SHM_CQI_BSR);
    shm_unlink(SHM_DCI);
    shm_unlink(SHM_SYNC_TIME);
    printf("Unlinking shared memory and semaphores...\n");
    // Free allocated memory
    // free(ue);
    // free(response);
    // free(transport_infomation);
    // free(buffer);
    // free(ue_data);
    // free(response_data);
    // free(transport_infomation);
    // free(ue);
    // free(response);
    // free(transport_infomation);
    // free(buffer);
    // free(ue_data);
    return 0;
}