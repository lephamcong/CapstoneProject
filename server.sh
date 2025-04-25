#!/bin/bash


# SCENARIORS=("high_traffic" "low_traffic" "burst_traffic" "ideal_condition" "mobility_pattern" "ideal_condition_bsr10000" "ideal_condition_bsr100000")
# TYPE_SCHEDULER=("rr" "max_cqi")

gcc -o clean ./Simulation/clean.c
./clean

gcc -o server ./Simulation/server.c ./Simulation/define.h
gcc -o client ./Simulation/client.c ./Simulation/define.h


./server "./Simulation/result/burst_traffic/pf.csv" pf
