import csv
import random
import math

NUM_TTI = 20000
NUM_UE = 12

def write_dual_scenario(base_name, cqi_matrix, bsr_matrix):
    with open(f'cqi_{base_name}.csv', 'w', newline='') as cqi_file, \
         open(f'bsr_{base_name}.csv', 'w', newline='') as bsr_file:

        cqi_writer = csv.writer(cqi_file)
        bsr_writer = csv.writer(bsr_file)

        header = [f'UE{i}' for i in range(NUM_UE)]
        cqi_writer.writerow(header)
        bsr_writer.writerow(header)

        for tti in range(NUM_TTI):
            cqi_writer.writerow([cqi_matrix[ue][tti] for ue in range(NUM_UE)])
            bsr_writer.writerow([bsr_matrix[ue][tti] for ue in range(NUM_UE)])

def random_cqi(min_val=1, max_val=15):
    return [[random.randint(min_val, max_val) for _ in range(NUM_TTI)] for _ in range(NUM_UE)]

def random_bsr(min_val=1000, max_val=10000):
    return [[random.randint(min_val, max_val) for _ in range(NUM_TTI)] for _ in range(NUM_UE)]

def scenario_idal_condition_bsr100000():
    cqi = []
    for ue in range(NUM_UE):
        if ue < 4:
            row = [random.randint(11, 15) for _ in range(NUM_TTI)]  # nhóm tốt
        elif ue < 8:
            row = [random.randint(6, 10) for _ in range(NUM_TTI)]   # nhóm trung bình
        else:
            row = [random.randint(1, 5) for _ in range(NUM_TTI)]    # nhóm xấu
        cqi.append(row)

    bsr = random_bsr(90000,100000)  # traffic raast cao
    return cqi, bsr


random.seed(42) 
write_dual_scenario("ideal_condition_bsr100000", *scenario_idal_condition_bsr100000())