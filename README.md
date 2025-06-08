<!-- PROJECT LOGO -->
<br />
<div align="center">
    <p align="center">
        <a  href="https://viettelhightech.vn/">
            <img src="https://viettelhightech.com/storage/icon/logo2.svg" alt="Logo Viettel High Tech" width="200" height="50">
        </a>
        <a href="http://ete.dut.udn.vn">
            <img src="http://ete.dut.udn.vn/theme/images/logo.jpg" alt="Logo ETE" height="70">
        </a>
    </p>

  <h2 align="center">CAPSTONE PROJECT</h2>
  <h3>Topic: Analysis and simulation of MAC layer resource scheduling algorithms in 5G NR</h3>
  <hr/>
</div>

## Information

### Students: 
- **Le Pham Cong** 
- **Huynh Nhat Anh**  

### Supervisors:
- **PhD. Van Phu Tuan** — Faculty of Electronics and Telecommunication Engineering, University of Science and Technology, University of Danang (DUT)
- **Engineer. Pham Khanh Trung** — Viettel High Technology Industries Corporation (VHT)

---

## Table of Contents

- [Description](#description)
- [Requirements](#requirements)
- [Run](#run)
- [Folder Structure](#folder-structure)
- [Result](#result)
- [Author](#author)
- [References](#references)
- [Appendix](#appendix)


## Description
This project simulates and evaluates the performance of different MAC layer resource scheduling algorithms in a 5G NR network environment, include:
* Round Robin
* Proportional Fair
* Max C/I
* Deep Q-Learning

The simulation computes and compares key performance indicators (KPIs) such as:

* Scheduler Count per UE
* Average Throughput
* Maximum Delay between transmissions
* Average Delay

## Requirements
* **Language:** C
* **Compiler:** GCC
* **Operating System:** POSIX-compiant OS (Linux)
* **Requirements:**
    * POSIX shared memory
    * POSIX semaphores
## Run
### Generate Data
```bash
cd Simulation/
cd data/
python3 ./gen_cqi_table.py
```
### Run each algorithm
```bash
cd Simulation/
cd ProportionalFair/

# Compile
gcc -o scheduler schedulerProportionalFair.c -lm
gcc -o ue_simulation ueSimulationProportionalFair.c

# Run with 2 process
./scheduler "./ProportionalFair_Result.csv"
./ue_simulation "./cqiData.csv" "./bsrData.csv"
```
To run other algorithm, replace ProportionalFair with MaxC_I, Qlearning or RoundRobin

## Folder Structure
```bash
CAPSTONEPROJECT/
├── Simulation/
│   ├── data/
│   │   ├── bsr_ideal_condition_bsr100000.csv        
│   │   ├── cqi_ideal_condition_bsr100000.csv           
│   │   └── gen_cqi_table.py 
│   ├── MaxC_I/
│   │   ├── bsrData.csv
│   │   ├── cqiData.csv
│   │   ├── MaxC_I_Result.csv
│   │   ├── scheduleMaxC_I.c
│   │   └── ueSimulationMaxC_I.c
│   ├── ProportionalFair/
│   │   ├── bsrData.csv
│   │   ├── cqiData.csv
│   │   ├── ProportionalFair_Result.csv
│   │   ├── scheduleProportionalFair.c
│   │   └── ueSimulationProportionalFair.c
│   ├── QLearning/
│   │   ├── model/
│   │   │   ├── dqn_model_v1.bin
│   │   │   ├── dqn_model_v2.bin
│   │   │   └── dqn_model.bin
│   │   └── result/
│   │       ├── bsrData.csv
│   │       ├── cqiData.csv
│   │       ├── DQL.c
│   │       ├── QLearning_Result_v1.csv
│   │       ├── QLearning_Result_v2.csv
│   │       ├── QLearning_Result.csv
│   │       ├── scheduleQLearning.c
│   │       └── ueSimulationQLearning.c
│   ├── result/
│   │   ├── max_c_i.csv
│   │   ├── pf.csv
│   │   ├── QLearning_Result_v1.csv
│   │   ├── QLearning_Result_v2.csv
│   │   ├── QLearning_Result.csv
│   │   ├── resultAnalysis_v1_deleted.ipynb
│   │   ├── resultAnalysis_v1.ipynb
│   │   └── rr.csv
│   ├── RoundRobin/
│   │   ├── bsrData.csv
│   │   ├── cqiData.csv
│   │   ├── RoundRobin_Result.csv
│   │   ├── scheduleRoundRobin.c
│   │   └── ueSimulationRoundRobin.c
│   ├── clean.c
│   ├── define.h
│   └──TBSArray.csv
├── TBS/
│   ├── define.h
│   ├── TBSArray.csv
│   ├── TBSArray.c
│   ├── 3gpp341-g40.pdf
│   ├── MCSIndexTable1.csv
│   ├── MCSIndexTable2.csv
│   ├── MCSIndexTable3.csv
│   └── TBS_calculate.c
├── .gitignore
└── readme.md
```
## Result
<p align="center">
  <img src="Simulation/result/each_ue.png" alt="Fig 1. Statistical chart by criteria for each user" width="600"/>
</p>
Fig 1. Statistical chart by criteria for each user

<p align="center">
  <img src="Simulation/result/cell.png" alt="Fig 2. Statistics chart of cell throughput and fairness" width="600"/>
</p>
Fig 2. Statistics chart of cell throughput and fairness


## References

<!-- ABOUT THE PROJECT -->