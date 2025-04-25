set -e

echo "Compiling UESimulation"
gcc -o UESimulation UESimulation.c

echo "Compiling Scheduler"
gcc -o Scheduler Scheduler.c roundrobin.c

echo "Running Scheduler"
./Scheduler

echo "Running UESimulation"
./UESimulation
