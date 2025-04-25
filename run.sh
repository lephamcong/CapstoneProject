#!/bin/bash

pkill server
pkill client

SCENARIORS=("high_traffic" "low_traffic" "burst_traffic" "ideal_condition" "mobility_pattern" "ideal_condition_bsr10000" "ideal_condition_bsr100000")
TYPE_SCHEDULER=("rr" "max_cqi" "pf")

gcc -o clean ./Simulation/clean.c
gcc -o server ./Simulation/server.c ./Simulation/define.h
gcc -o client ./Simulation/client.c ./Simulation/define.h

mkdir -p result

for SCENARIO in "${SCENARIORS[@]}"; do
    # Compile the server.c file

    mkdir -p "Simulation/result/$SCENARIO"

    for TYPE in "${TYPE_SCHEDULER[@]}"; do
        ./clean
        echo "Running server with scheduler type: $TYPE with scenario: $SCENARIO"
        ./server "./Simulation/result/$SCENARIO/$TYPE.csv" $TYPE &
        ./client "./Simulation/data/cqi_$SCENARIO.csv" "./Simulation/data/bsr_$SCENARIO.csv" 
        wait
        echo "Complete: $SCENARIO with scheduler type: $TYPE"
    done
done

echo "Done."


# if (strcmp(type_scheduler,"rr") == 0) scheduler_type = ROUND_ROBIN; // Default scheduler type
#     else if (strcmp(type_scheduler,"max_cqi") == 0) scheduler_type = MAX_CQI;
#     else if (strcmp(type_scheduler,"pf") == 0) scheduler_type = PROPORTIONAL_FAIR;
#     else if (strcmp(type_scheduler,"q_learning") == 0) scheduler_type = Q_LEARNING;
#     else {
#         LOG_ERROR("Invalid scheduler type");
#         return 1;
#     }