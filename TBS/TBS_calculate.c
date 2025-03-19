/**
 * @file TBS_calculate.c
 * @brief This file contains functions to read MCS Index table from a CSV file, calculate Transport Block Size (TBS) based on given parameters, and write the TBS array to a CSV file.
 * 
 * The main functionalities include:
 * - Reading MCS Index table from a CSV file.
 * - Calculating TBS for different MCS Index and Resource Blocks (RB).
 * - Writing the calculated TBS array to a CSV file.
 * 
 * The file defines several macros and constants used in the calculations and file operations.
 * 
 * Macros:
 * - max(a, b): Returns the maximum of two values.
 * - min(a, b): Returns the minimum of two values.
 * - LOG_ERROR(msg): Logs an error message with file name, line number, and errno.
 * 
 * Constants:
 * - MAX_TABLE: Maximum size of the table.
 * - MAX_LINE: Maximum length of a line in the file.
 * - MAX_MCSIndex: Maximum MCS Index.
 * - MAX_RB: Maximum Resource Blocks.
 * - NUM_MACTable: Number of MCS tables.
 * - NUM_LAYER: Number of layers.
 * 
 * Functions:
 * - void read_file(char* filename, float matrix[MAX_TABLE][MAX_TABLE], int* rows, int* cols): Reads a CSV file and stores the data in a matrix.
 * - void write_file(const char *filename, int array[28][100]): Writes the TBS array to a CSV file.
 * - int calc_TBS(int Q_m, int R_m, int Nsh_symb, int Nprb_dmrs, int Nprb_oh, int Nprb, int v): Calculates the Transport Block Size (TBS) based on given parameters.
 * - int main(): Main function that reads the MCS Index table, calculates the TBS array, and writes the TBS array to a CSV file.
 * 
 * The main function performs the following steps:
 * - Reads the MCS Index table from a CSV file.
 * - Calculates the TBS for different MCS Index and Resource Blocks.
 * - Writes the calculated TBS array to a CSV file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#define max(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
({ __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
_a < _b ? _a : _b; })

#define LOG_ERROR(msg) fprintf(stderr, "[%s:%d] %s: errno=%d (%s)\n", __FILE__, __LINE__, msg, errno, strerror(errno))

#define MAX_TABLE 105
#define MAX_LINE 100

#define MAX_MCSIndex 28
#define MAX_RB 100
#define NUM_MACTable 1 
#define NUM_LAYER 1

void read_file(char* filename, float matrix[MAX_TABLE][MAX_TABLE], int* rows, int* cols){
    FILE *file;
    file = fopen(filename, "r");
    if (file == NULL) {
        LOG_ERROR("Cannot open file");
        return;
    }
    // read the first line
    char line[MAX_LINE];
    *rows = 0;
    fgets(line, sizeof(line), file);
    
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ","); // split the line by comma (,)
        *cols = 0;
        while (token) {
            matrix[*rows][*cols] = atof(token); // convert string to float
            token = strtok(NULL, ",");  // get the next token
            (*cols)++;
        }
        (*rows)++;
    }

    fclose(file); // close the file
}

void write_file(const char *filename, int array[28][100]) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        LOG_ERROR("Cannot open file");
        return;
    }

    fprintf(file, "MCS/RB"); 
    for (int i = 1; i <= MAX_RB; i++) {
        fprintf(file, ",%d", i); 
    }
    fprintf(file, "\n");

    for (int j = 0; j < MAX_MCSIndex; j++) {
        fprintf(file, "%d", j); 
        for (int i = 1; i <= MAX_RB; i++) {
            fprintf(file, ",%d", array[j][i]); 
        }
        fprintf(file, "\n"); 
    }

    fclose(file);
    printf("Writing done to file %s", filename);
}

int calc_TBS(int Q_m, int R_m, int Nsh_symb, int Nprb_dmrs, int Nprb_oh, int Nprb, int v) {
    /*
    Input: 
    - Q_m: Modulation order
    - R_m: Target code rate x 1024
    - Nrb_sc = 12: Number of subcarriers in a physical resource block
    - Nsh_symb: Number of symbols of the PDSCH allocation in a slot
    - Nprb_dmrs: Number of REs for DM-RS per PRB
    - Nprb_oh: Number of PRB for overhead configured by higher layer parameter xOverhead
    - Nprb: Total number of allocated PRBs to the UE
    - v: Number of layers
    Output:
    - TBS: Transport block size
    */

    float Nrb_sc = 12;
    float Nre_;
    float Nre;
    float Ninfo;
    float Ninfo_;
    float n;
    float C;
    int TBStable[93] = {
        24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176,
        184, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 408, 432, 456, 480,
        504, 528, 552, 576, 608, 640, 672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064,
        1128, 1160, 1192, 1224, 1256, 1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800,
        1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856,
        2976, 3104, 3240, 3368, 3496, 3624, 3752, 3824
    };
    Nre_ = Nrb_sc * Nsh_symb - Nprb_dmrs - Nprb_oh;
    Nre = min (156, Nre_) * Nprb;
    Ninfo = Nre * Q_m * ((float) R_m/1024) * v;
    if (Ninfo <= 3824.0){
        n = max(3, (floor(log2((float)Ninfo)) - 6));
        Ninfo_ = max(24, pow(2,n) * floor((float)Ninfo / (pow(2,n))));
        for(int i = 0; i < 93; i++) {
            if (Ninfo_ <= TBStable[i]) {
                return TBStable[i];
            }
        }
    } else {
        n = floor(log2(Ninfo - 24)) - 5;
        Ninfo_ = max(3840,pow(2,n) * round((Ninfo - 24) / (pow(2,n))));
        if (((float)R_m/1024) <= 0.25){
            C = ceil((Ninfo_ + 24) / 3816);
            return (int)8 * C * ceil((Ninfo_ + 24) / (8 * C)) - 24;
        } else {
            if (Ninfo_ > 8424){
                C = ceil((Ninfo_ + 24) / 8424);
                return (int)8 * C * ceil((Ninfo_ + 24) / (8 * C)) - 24;
            }
            else {
                return (int)8 * ceil((Ninfo_ + 24) / 8) - 24;
            }
        }
    };

}

int main() {
    int TBSArray[MAX_MCSIndex][MAX_RB];
    float MCSTable[MAX_TABLE][MAX_TABLE]; // Mảng chứa bảng MCS Index
    int rows_MCSTable, cols_MCSTable; // Số hàng và số cột của bảng MCS Index
    char* filename = malloc(30);

    sprintf(filename, "MCSIndexTable%d.csv", NUM_MACTable);
    read_file(filename, MCSTable, &rows_MCSTable, &cols_MCSTable);

    // for (int i = 0; i < rows_MCSTable; i++) {
    //     for (int j = 0; j < cols_MCSTable; j++) {
    //         printf("%.4f ", MCSTable[i][j]);
    //     }
    //     printf("\n");
    // }

    int Nsh_symb = 14;
    int Nprb_dmrs = 12*2;
    int Nprb_oh = 0;

    for(int i = 0; i < MAX_MCSIndex; i++) {
        for(int j = 1; j <= MAX_RB; j++) {
            TBSArray[i][j] = calc_TBS(MCSTable[i][1], MCSTable[i][2], Nsh_symb, Nprb_dmrs, Nprb_oh, j, 1);
        }
    }
    printf("----------------------- \n")    ;
    for(int i = 0; i < MAX_MCSIndex; i++) {
        printf("\nMCS=%d,",i);
        for(int j = 1; j <= MAX_RB; j++) {
            printf("%d,", TBSArray[i][j]);
        }
    }
    write_file("TBSArray.csv", TBSArray);

    return 0;
}
