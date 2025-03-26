#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/ue_controller"


#define LOG_ERROR(msg) fprintf(stderr, "[%s:%d] %s: errno=%d (%s)\n", __FILE__, __LINE__, msg, errno, strerror(errno))

#define UE_PORT 5501
#define CONTROLLER_PORT 5500
#define CONTROLLER_IP "192.168.1.32"

// Define arguments for MAC Scheduler
#define NUM_UE 12
#define NUM_RB 100
#define MAX_MAC_INDEX 28
// #define TBSArray[MAX_MAC_INDEX][NUM_RB]
#define NUM_LAYER 1
#define MAX_UE_PER_TTI 4
#define SUBFRAME_DURATION 1


// Define some functions
#define max(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
_a < _b ? _a : _b; })

