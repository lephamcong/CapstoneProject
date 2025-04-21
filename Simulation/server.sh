#!/bin/bash

gcc clean.c -o clean
gcc server.c -o server 

./clean
./server 