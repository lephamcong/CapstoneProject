/*
    * File:   UESimulation.c
    * Compile: gcc -o UESimulation UESimulation.c
    * Run: ./UESimulation <IP Address> <UE ID> 
*/

#include "define.h"


int main(int argc, char **argv) {
    if (argc != 3) {
        LOG_ERROR("Usage: ./UESimulation <IP Address> <UE ID>");
        return 1;
    }

    int ueID = atoi(argv[2]);

            

    return 0;
}

