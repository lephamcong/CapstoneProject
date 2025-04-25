#!/bin/bash


# SCENARIORS=("high_traffic" "low_traffic" "burst_traffic" "ideal_condition" "mobility_pattern" "ideal_condition_bsr10000" "ideal_condition_bsr100000")
# TYPE_SCHEDULER=("rr" "max_cqi")

gcc -o clean clean.c
./clean

gcc -o server server.c define.h
gcc -o client client.c define.h


./server "results/burst_traffic/pf.csv" pf