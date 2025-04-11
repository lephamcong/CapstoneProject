
#define _POSIX_C_SOURCE 199309L
#include <stdio.h> // standard input/output library
#include <stdlib.h> // standard library of C
#include <sys/socket.h> // socket library
#include <netinet/in.h> // internet address family
#include <string.h> // string manipulation library
#include <unistd.h> // POSIX operating system API
#include <arpa/inet.h> // internet address manipulation library
#include <pthread.h> // POSIX threads library
#include <time.h> // time manipulation library
#include <math.h>  // math library
#include <errno.h> // error number library
#include <fcntl.h> // file control library
#include <sys/mman.h> // memory management declarations
#include <semaphore.h> // semaphore library
#include <sys/stat.h> // file status library


#ifndef HEADERFILE_H // header guard
#define HEADERFILE_H // header guard

// LOG_ERROR macro to print error messages
#define LOG_ERROR(msg) fprintf(stderr, "[%s:%d] %s: errno=%d (%s)\n", __FILE__, __LINE__, msg, errno, strerror(errno))

//Define arguments for UDP socket
#define UE_PORT 5501
#define SERVER_PORT 5500
#define SERVER_IP "127.0.0.1"

// Define arguments for MAC Scheduler
#define NUM_UE 12
#define NUM_RB 100
#define MAX_MCS_INDEX 28
#define NUM_LAYER 1
#define MAX_UE_PER_TTI 4
#define SUBFRAME_DURATION 1
#define NUM_TTI 10
#define NUM_TTI_RESEND 4


// Define some functions
#define max(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
_a < _b ? _a : _b; })

// Define Structure for UE Data

typedef struct
{
    int tti;
} TransInfo;

// void roundrobin(UEData* ue, int* TBSize);


#define SHM_CQI_BSR   "/shm_cqi_bsr"
#define SHM_DCI       "/shm_dci"
#define SHM_SYNC_TIME "/shm_sync_time"
#define SEM_CQI       "/sem_cqi"
#define SEM_DCI       "/sem_dci"
#define SEM_SYNC      "/sem_sync"
#define MAX_UE 12
#define TTI_PERIOD_NS 1000000L // 1ms

typedef struct {
    int id;
    int cqi;
    int bsr;
} UEData;

// Dữ liệu DCI từ Scheduler
typedef struct {
    int id;
    int tb_size;
} SchedulerResponse;

// Thời điểm Scheduler bắt đầu chạy
typedef struct {
    long start_time_ms;
} SyncTime;

#endif
