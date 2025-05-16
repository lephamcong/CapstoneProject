import csv
import random
import math

NUM_TTI = 10000
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

# ================= KỊCH BẢN ===================

def random_cqi(min_val=1, max_val=15):
    return [[random.randint(min_val, max_val) for _ in range(NUM_TTI)] for _ in range(NUM_UE)]

def random_bsr(min_val=1000, max_val=10000):
    return [[random.randint(min_val, max_val) for _ in range(NUM_TTI)] for _ in range(NUM_UE)]

# 1. high_traffic: CQI dao động nhẹ, BSR cao
def scenario_high_traffic():
    return random_cqi(6, 9), random_bsr(80000, 100000)

# 2. low_traffic: CQI cao, BSR thấp
def scenario_low_traffic():
    return random_cqi(10, 15), random_bsr(10000, 20000)

# 3. burst_traffic: BSR tăng đột ngột theo chu kỳ
def scenario_burst_traffic():
    cqi = random_cqi(6, 10)
    bsr = []
    for ue in range(NUM_UE):
        row = []
        for tti in range(NUM_TTI):
            if tti % 500 < 100:
                row.append(random.randint(80000, 100000))  # burst
            else:
                row.append(random.randint(10000, 30000))
        bsr.append(row)
    return cqi, bsr

# 4. mobility_pattern: CQI thay đổi theo sin, BSR ngẫu nhiên
def scenario_mobility_pattern():
    cqi = []
    for ue in range(NUM_UE):
        freq = 0.0005 + ue * 0.0001
        row = [int(8 + 6 * math.sin(2 * math.pi * freq * tti)) for tti in range(NUM_TTI)]
        cqi.append(row)
    bsr = random_bsr(30000, 70000)
    return cqi, bsr

# 5. mixed: mỗi UE có kiểu hành vi khác nhau
# def scenario_mixed():
#     cqi = []
#     bsr = []
#     for ue in range(NUM_UE):
#         # CQI mỗi UE khác nhau
#         cqi_freq = 0.001 * (ue + 1)
#         cqi_row = [int(7 + 5 * math.sin(cqi_freq * tti)) for tti in range(NUM_TTI)]
#         cqi.append(cqi_row)

#         if ue % 3 == 0:
#             bsr_row = [random.randint(80, 100) for _ in range(NUM_TTI)]
#         elif ue % 3 == 1:
#             bsr_row = [int(50 + 30 * math.sin(0.001 * tti)) for tti in range(NUM_TTI)]
#         else:
#             bsr_row = [random.randint(0, 20) if tti % 1000 < 900 else random.randint(80, 100) for tti in range(NUM_TTI)]
#         bsr.append(bsr_row)

#     return cqi, bsr

# 6. ideal_condition: CQI chia làm 3 nhóm (tốt, trung bình, xấu)
def scenario_ideal_condition():
    cqi = []
    for ue in range(NUM_UE):
        if ue < 4:
            row = [random.randint(11, 15) for _ in range(NUM_TTI)]  # nhóm tốt
        elif ue < 8:
            row = [random.randint(6, 10) for _ in range(NUM_TTI)]   # nhóm trung bình
        else:
            row = [random.randint(1, 5) for _ in range(NUM_TTI)]    # nhóm xấu
        cqi.append(row)
    
    bsr = random_bsr(40000, 70000)  # traffic vừa phải
    return cqi, bsr

def scenario_ideal_condition_bsr10000():
    cqi = []
    for ue in range(NUM_UE):
        if ue < 4:
            row = [random.randint(11, 15) for _ in range(NUM_TTI)]  # nhóm tốt
        elif ue < 8:
            row = [random.randint(6, 10) for _ in range(NUM_TTI)]   # nhóm trung bình
        else:
            row = [random.randint(1, 5) for _ in range(NUM_TTI)]    # nhóm xấu
        cqi.append(row)

    bsr = random_bsr(9000,10000)  # traffic vừa phải
    return cqi, bsr

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
# ================= VIẾT RA FILE ===================
random.seed(42) 
write_dual_scenario("high_traffic", *scenario_high_traffic())
write_dual_scenario("low_traffic", *scenario_low_traffic())
write_dual_scenario("burst_traffic", *scenario_burst_traffic())
write_dual_scenario("mobility_pattern", *scenario_mobility_pattern())
write_dual_scenario("ideal_condition", *scenario_ideal_condition())
write_dual_scenario("ideal_condition_bsr10000", *scenario_ideal_condition_bsr10000())
write_dual_scenario("ideal_condition_bsr100000", *scenario_idal_condition_bsr100000())
