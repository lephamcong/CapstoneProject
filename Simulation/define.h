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

/*-----------------------------------------------------------------------*/
// Config here
#define PROCESS 1
// Only for Deep Q Learning : 0 train, 1 test

#define TIME_FRAME 20
#define ENABLE_LOG 0
#define TBS_FILE "../TBSArray.csv"
// #define CQI_FILE "/home/kaonashi/CapstoneProject/CapstoneProject/gen_cqi/cqi_ideal_condition.csv"
// #define BSR_FILE "/home/kaonashi/CapstoneProject/CapstoneProject/gen_cqi/bsr_ideal_condition.csv"
// #define LOG_FILE "/home/kaonashi/CapstoneProject/CapstoneProject/ideal_condition/log_TBS_cqi.csv"

enum SCHEDULER_TYPE {
    ROUND_ROBIN,
    MAX_CQI,
    PROPORTIONAL_FAIR,
    Q_LEARNING
};


/*-----------------------------------------------------------------------*/
// ANSI color codes
#define COLOR_RESET    "\033[0m"
#define COLOR_RED      "\033[1;31m"
#define COLOR_YELLOW   "\033[1;33m"
#define COLOR_GREEN    "\033[1;32m"
#define COLOR_BLUE     "\033[1;34m"
/*-----------------------------------------------------------------------*/
// Macros for logging
#if ENABLE_LOG
#define LOG_ERROR(fmt, ...) fprintf(stderr, COLOR_RED    "[ERROR] [%s:%d] " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  fprintf(stderr, COLOR_YELLOW "[WARN ] [%s:%d] " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  fprintf(stdout, COLOR_BLUE   "[INFO ] [%s:%d] " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_OK(fmt, ...)    fprintf(stdout, COLOR_GREEN  "[ OK  ] [%s:%d] " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#else
#define LOG_ERROR(fmt, ...) fprintf(stderr, COLOR_RED    "[ERROR] [%s:%d] " fmt COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_OK(fmt, ...)
#endif
/*-----------------------------------------------------------------------*/
// Define arguments for MAC Scheduler
#define NUM_UE 12
#define NUM_RB 100
#define NUM_RB_PER_UE 25
#define MAX_MCS_INDEX 28
#define NUM_LAYER 1
#define MAX_UE_PER_TTI 4
#define SUBFRAME_DURATION 1

#if PROCESS == 1
#define NUM_TTI 10000
#else 
#define NUM_TTI 20000
#endif

#define NUM_TTI_RESEND 10
/*-----------------------------------------------------------------------*/
// Define shared memory and semaphore names
#define SHM_CQI_BSR   "/shm_cqi_bsr"
#define SHM_DCI       "/shm_dci"
#define SHM_SYNC_TIME "/shm_sync_time"
#define SEM_UE_SEND       "/sem_ue_send"
#define SEM_UE_RECV       "/sem_ue_recv"
#define SEM_SCHEDULER_SEND "/sem_scheduler_send"
#define SEM_SCHEDULER_RECV "/sem_scheduler_recv"
#define SEM_SYNC      "/sem_sync"
#define MAX_UE 12
#define TTI_PERIOD_NS 1000000L // 1ms
/*-----------------------------------------------------------------------*/
// Define structures for UE data and transport information
typedef struct {
    int id;
    int cqi;
    int bsr;
} UEData;
// Define structure for Scheduler response
typedef struct {
    int id;
    int tb_size;
} SchedulerResponse;

// define structure for Sync time
typedef struct {
    long start_time_ms;
} SyncTime;

/*-----------------------------------------------------------------------*/
// Define some functions
#define max(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
_a < _b ? _a : _b; })

// Function to format time as a string
static inline const char* format_time_str(long time) {
    time_t t = time / 1000;
    struct tm* timeinfo = localtime(&t);
    static char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    time_str[sizeof(time_str) - 1] = '\0'; // Ensure null-termination
    return time_str;
}

static inline uint64_t get_elapsed_ms(uint64_t start) {
    /*
        Function to calculate elapsed time in milliseconds (ms)
        since the start time.
        Parameters:
            start:  The start time in nanoseconds (ns).
        Returns:
            Elapsed time in milliseconds (ms).
    */
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    uint64_t now_ms = (uint64_t)now.tv_sec * 1e3 + now.tv_nsec / 1e6;
    return (now_ms - start);
}

static inline uint64_t get_current_time_ms() {
    /*
        Function to get the current time in milliseconds (ms).
        Returns:
            Current time in milliseconds (ms).
    */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}


static inline uint64_t get_elapsed_tti_frame(uint64_t start) {
    /*
        Function to calculate elapsed time in milliseconds (ms)
        since the start time.
        Parameters:
            start:  The start time in nanoseconds (ns).
        Returns:
            Elapsed time in milliseconds (ms).
    */
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    uint64_t now_ms = ((uint64_t)now.tv_sec * 1e3 + now.tv_nsec / 1e6) / TIME_FRAME;
    return (now_ms - start);
}

static inline uint64_t get_current_tti_frame() {
    /*
        Function to get the current time in milliseconds (ms).
        Returns:
            Current time in milliseconds (ms).
    */
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t) (ts.tv_sec * 1e3 + ts.tv_nsec / 1e6) / TIME_FRAME;
}

static inline int cqi_to_mcs(int cqi) {
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

#define MAX_LINE_LENGTH 1024    // Maximum length of a line in the CSV file
#define MAX_COLUMNS 1000        // Maximum number of columns in the CSV file

int TBS[MAX_MCS_INDEX][NUM_RB];
// Function to load TBS (Transport Block Size) table from CSV file
static inline void TBS_Table() {
    /*
        Function to load TBS (Transport Block Size) table from CSV file.
        Returns:
            None
    */
    FILE *file = fopen(TBS_FILE, "r");
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

// Function to log Transport Block Size (TBS) to a file
static inline void log_tbsize(
    FILE *log_file,                 // Pointer to the log file.
    int tti,                        // TTI value.
    SchedulerResponse *response,    // Pointer to the SchedulerResponse structure.
    int num_ue                      // Number of UEs.
) {
    /*
        Function to log the TB size for each UE at a given TTI.
        
        Parameters:
            log_file: Pointer to the log file.
            tti: TTI value.
            response: Pointer to the SchedulerResponse structure.
            num_ue: Number of UEs.
        
        Returns:
            None
    */
    fprintf(log_file, "\n%d", tti);
    for (int i = 0; i < num_ue; i++) {
        fprintf(log_file, ",%d", response[i].tb_size);
    }
};


/*-----------------------------------------------------------------------*/
// Function to write data to a CSV file
static inline void write_csv(const char *filename, const char *data) {
    /*
        Function to write data to a CSV file.
        
        Parameters:
            filename:  Name of the CSV file.
            data:      Data to be written to the CSV file.
    */
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        LOG_ERROR("Error opening file for writing");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%s\n", data);
    fclose(file);
}
// Function to read data from a CSV file
#define MAX_LINE_LENGTH 1024

static inline void read_csv(const char *filename) {
    /*
        Function to read data from a CSV file.
        
        Parameters:
            filename:  Name of the CSV file.
    */
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        LOG_ERROR("Error opening file for reading");
        exit(EXIT_FAILURE);
    }
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char *token;
        token = strtok(line, ",");
        while (token != NULL) {
            printf("%s ", token);
            token = strtok(NULL, ",");
        }
        printf("\n");
    }
    fclose(file);
}

#endif