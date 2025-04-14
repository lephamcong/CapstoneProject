/*
    Round Robin Scheduling Algorithm
    -----------
    Description:
        -   UEs (User Equipment) are arranged in a sequential queue.
        -   At each TTI (1ms), the Scheduler will select the next group of UEs in turn (e.g. 4 UEs per TTI).
        -   After reaching the end of the list, the algorithm will go back to the beginning (round).
    -----------
    Goal:
        -   Ensure all UEs are scheduled in turn, without favoring any UE.
    -----------
    Input:
        -   UEData[]: id, cqi, bsr each UE
        -   MAX_UE: maximum number of UEs
        -   MAX_UE_PER_TTI: maximum number of UEs per TTI
        -   MAX_RB: maximum number of RBs (Resource Blocks)
        -   TBS[MCS][RB]: Transport Block Size (TBS) table
    Output:
        -   SchedulerResponse[]: id, tb_size each UE
    -----------  
*/

#include "./define.h"



int main() {

    return 0;
}