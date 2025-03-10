# Tính bảng TBS (Transport Block Size) từ MCS và Resource Block (RB)

## Cấu trúc file

+ **MCSIndexTable1.csv**: Bảng tra cứu chỉ số MCS cho PDSCH thứ nhất theo 3GPP TS 38.214 
+ **MCSIndexTable2.csv**: Bảng tra cứu chỉ số MCS cho PDSCH thứ hai theo 3GPP TS 38.214 
+ **MCSIndexTable3.csv**: Bảng tra cứu chỉ số MCS cho PDSCH thứ ba theo 3GPP TS 38.214 
+ **TBS_calculate.c**: Chương trình tính bảng TBS
+ **TBSArray.csv**: File chứa kết quả là bảng TBS đã tính được
 
## Công thức tính

### Đầu vào

+ **Q_m**: Modulation order
+ **R_m**: Target code rate x 1024
+ **Nrb_sc** = 12: Number of subcarriers in a physical resource block
+ **Nsh_symb**: Number of symbols of the PDSCH allocation in a slot
+ **Nprb_dmrs**: Number of REs for DM-RS per PRB
+ **Nprb_oh**: Number of PRB for overhead configured by higher layer parameter xOverhead
+ **Nprb**: Total number of allocated PRBs to the UE
+ **v**: Number of layers

### Bước 1

$$ N'_{RE} = N^{RB}_{sc} \cdot N^{sh}_{symb} - N^{PRB}_{DMRS} - N^{PRB}_{oh}$$

$$N_{RE} = min(156, N'_{RE}) \cdot n_{PRB}$$

### Bước 2

$$N_{info} = N_{RE} \cdot R \cdot Q_m \cdot v$$
Nếu $N_{info} \leq 3824 $ thì sang **Bước 3**, ngược lại sang **Bước 4**

### Bước 3

$$N'_{info} = max\left(24, 2^n\cdot \left\lfloor \frac{N_{info}}{2^n} \right\rfloor\right)$$ 
với $n = max(3, \lfloor log_2(N_{info})\rfloor-6)$

Sử dụng bảng 5.1.3.2-1 để xác định giá trị TBS gần nhất không nhỏ hơn $N'_{info}$

### Bước 4

$$N'_{info} = max\left(3840, 2^n \times round \left( \frac{N_{info - 24}}{2^n}\right) \right)$$
với $n = \lfloor log_2(N_{info}-24)\rfloor-5$

Nếu $R\leq 1/4$
$$TBS = 8 \cdot C \cdot \left\lceil \frac{N_{info} + 24}{8 \cdot C}\right\rceil-24$$
với $C = \left\lfloor \frac{N_{info}+24}{3816} \right\rfloor$

Ngược lại
    
Nếu $N'_{info} > 8424$ 

$$TBS = 8 \cdot C \cdot \left\lceil \frac{N_{info} + 24}{8 \cdot C}\right\rceil-24$$
với $C = \left\lfloor \frac{N_{info}+24}{8424} \right\rfloor$

Ngược lại 
$$TBS = 8 \cdot \left\lceil \frac{N'_{info}+24}{8}  \right\rceil- 24$$
