#!/bin/bash

# gcc -o clean clean.c
# gcc -o server server.c define.h
# gcc -o client client.c define.h


# ./client "gen_cqi/cqi_burst_traffic.csv" "gen_cqi/bsr_burst_traffic.csv" 
./client "./Simulation/data/cqi_burst_traffic.csv" "./Simulation/data/bsr_burst_traffic.csv" 