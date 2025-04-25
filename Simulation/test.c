#include "define.h"

#define MAX_LINE_LENGTH 1024
#define MAX_COLUMNS 1000

int TBS[MAX_MCS_INDEX][NUM_RB];

void TBSTable() {
    FILE *file = fopen("./TBSArray.csv", "r");
    if (file == NULL) {
        LOG_ERROR("Error opening TBS file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE_LENGTH];
    int i = 0;

    fgets(line, sizeof(line), file); // Read the header line

    while (fgets(line, sizeof(line), file) && i < MAX_MCS_INDEX) {
        char *token = strtok(line, ",");  // Token đầu tiên là MCS index, bỏ qua
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

int main() {
    
    TBSTable();
    printf("TBS[0][0] = %d\n", TBS[0][0]);  // MCS 0, RB 1
    printf("TBS[27][99] = %d\n", TBS[27][99]);  // MCS 27, RB 100

    return 0;
}
