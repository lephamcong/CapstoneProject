#include "define.h"

void roundrobin(UEData* ue, int* TBSize){
    static int countRR = 0;
    int count;
    int TB[NUM_UE] = {0};
    int cur_UE;
    for (int i=1; i<=MAX_UE_PER_TTI; i++) {
        // cur_UE = count++ % 12;
        countRR++;
        while (ue[countRR%12].bsr == 0) countRR+=1; 
        // if (ue[cur_UE].bsr != 0)
        // {
        TB[countRR%12] += 25;
        printf("Scheduled for %d\n", ue[countRR%12].id);
        // }
    }
    printf("---------------------\n");
    for (int i=0; i<NUM_UE; i++) {
        TBSize[i] = TB[i]*100; //gan gia tri, se thay doi bang mapping tu tb va mcs
    }

}