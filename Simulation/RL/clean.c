#include "define.h"

int main(){
    // Unlink shared memory and semaphores
    if (sem_unlink(SEM_UE_SEND) == -1) {
        LOG_ERROR("Failed to unlink semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data unlinked successfully");
    }
    if (sem_unlink(SEM_UE_RECV) == -1) {
        LOG_ERROR("Failed to unlink semaphore for UE data");
        return 1;
    } else {
        LOG_OK("Semaphore for UE data unlinked successfully");
    }
    if (sem_unlink(SEM_SCHEDULER_SEND) == -1) {
        LOG_ERROR("Failed to unlink semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse unlinked successfully");
    }
    if (sem_unlink(SEM_SCHEDULER_RECV) == -1) {
        LOG_ERROR("Failed to unlink semaphore for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Semaphore for SchedulerResponse unlinked successfully");
    }
    if (sem_unlink(SEM_SYNC) == -1) {
        LOG_ERROR("Failed to unlink semaphore for sync");
        return 1;
    } else {
        LOG_OK("Semaphore for sync unlinked successfully");
    }
    if (shm_unlink(SHM_CQI_BSR) == -1) {
        LOG_ERROR("Failed to unlink shared memory for UE data");
        return 1;
    } else {
        LOG_OK("Shared memory for UE data unlinked successfully");
    }
    if (shm_unlink(SHM_DCI) == -1) {
        LOG_ERROR("Failed to unlink shared memory for SchedulerResponse");
        return 1;
    } else {
        LOG_OK("Shared memory for SchedulerResponse unlinked successfully");
    }
    if (shm_unlink(SHM_SYNC_TIME) == -1) {
        LOG_ERROR("Failed to unlink shared memory for sync");
        return 1;
    } else {
        LOG_OK("Shared memory for sync unlinked successfully");
    }
    printf("Unlinking shared memory and semaphores...\n");
    return 0;
}