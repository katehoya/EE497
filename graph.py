import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# CSV 파일 불러오기
df = pd.read_csv("bms_log2.csv")  # 파일 이름 맞게 수정
df = df.iloc[299:]   # 앞의 5000행만 사용

#start_index = 
#end_index = 4423

# 각 열 추출
time_log = df["time_min"].values
volt_logs = [
    df["cell1_V"].values,
    df["cell2_V"].values,
    df["cell3_V"].values,
    df["cell4_V"].values
]

# moving average 함수
def moving_average(data, window_size=50):
    return np.convolve(data, np.ones(window_size)/window_size, mode='valid')

# Plot
plt.figure(figsize=(12, 6))

cell_names = ["Cell 1", "Cell 2", "Cell 3", "Cell 4"]
colors = ["blue", "orange", "green", "red"]

for i, cell_data in enumerate(volt_logs):
    smooth = moving_average(cell_data, window_size=50)
    smooth_time = time_log[:len(smooth)]
    plt.plot(smooth_time, smooth, color=colors[i], label=f"{cell_names[i]}")
plt.ylim(3.0, 3.9)  # y축 범위를 3.0V ~ 3.8V로 고정
plt.xlabel("Time [min]", fontsize=13)
plt.ylabel("Voltage [V]", fontsize=13)
plt.title("Cell Voltage per Time. (BMS O)", fontsize=20, fontweight='bold')
plt.grid(True)
plt.legend(fontsize=15)
plt.tight_layout()
plt.show()
