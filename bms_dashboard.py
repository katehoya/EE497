import serial
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import time
import numpy as np
import pandas as pd

# --------- 설정 ----------
SERIAL_PORT = "COM4"   # ← 아두이노 포트로 변경
BAUD_RATE   = 9600
NUM_CELLS   = 4
MIN_V = 3.0
MAX_V = 4.3

# --------- 시리얼 포트 열기 ----------
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# --------- 대시보드 초기화 ----------
plt.ion()  # 인터랙티브 모드
fig, ax = plt.subplots(figsize=(8, 4))
fig.canvas.manager.set_window_title("BMS Cell Voltage Dashboard (Live)")

voltages = [0.0] * NUM_CELLS
x = list(range(1, NUM_CELLS + 1))

bars = ax.bar(x, voltages)
texts = []

for i, bar in enumerate(bars):
    height = bar.get_height()
    t = ax.text(
        bar.get_x() + bar.get_width() / 2,
        height + 0.03,
        f"{voltages[i]:.3f} V",
        ha='center',
        va='bottom',
        fontsize=10
    )
    texts.append(t)

ax.set_ylim(MIN_V, MAX_V + 0.2)
ax.yaxis.set_major_locator(ticker.MultipleLocator(0.2))
ax.set_xticks(x)
ax.set_xticklabels([f"Cell {i}" for i in range(NUM_CELLS)])
ax.set_ylabel("Voltage [V]")
ax.set_title("BMS Cell Voltages (Live)")

ax.axhline(MIN_V, linestyle="--", linewidth=1)
ax.axhline(MAX_V, linestyle="--", linewidth=1)

plt.tight_layout()
plt.show()

# --------- 로그용 변수들 (그래프용) ----------
t0 = time.time()              # 시작 시각
time_log = []                 # 시간 [min]
volt_logs = [[] for _ in range(NUM_CELLS)]  # 셀별 전압 로그
soc_logs  = [[] for _ in range(NUM_CELLS)]  # 셀별 SOC 로그 (0~1)

def voltage_to_soc(v, v_min=MIN_V, v_max=MAX_V):
    """가장 단순한 전압 → SOC 매핑 (선형, 0~1)"""
    soc = (v - v_min) / (v_max - v_min)
    return float(np.clip(soc, 0.0, 1.0))

# --------- 메인 루프 ----------
try:
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            plt.pause(0.01)
            continue

        # "DATA,4.123,4.095,4.110,4.080" 만 처리
        if line.startswith("DATA"):
            parts = line.split(',')
            if len(parts) >= NUM_CELLS + 1:
                try:
                    vals = [float(x) for x in parts[1:NUM_CELLS+1]]
                    voltages = vals
                    # ----- 로그 저장 (추가) -----
                    now_min = (time.time() - t0) / 60.0
                    time_log.append(now_min)
                    for i, v in enumerate(voltages):
                        volt_logs[i].append(v)
                        soc_logs[i].append(voltage_to_soc(v))

                    # 막대 / 텍스트 업데이트
                    for i, bar in enumerate(bars):
                        bar.set_height(voltages[i])
                        texts[i].set_text(f"{voltages[i]:.3f} V")
                        texts[i].set_y(voltages[i] + 0.03)

                    fig.canvas.draw()
                    fig.canvas.flush_events()

                except ValueError:
                    # 숫자 파싱 실패하면 그냥 스킵
                    pass

        plt.pause(0.01)  # UI 이벤트 처리용

except KeyboardInterrupt:
    print("종료")
finally:
        ser.close()
        plt.ioff()  # 인터랙티브 모드 해제

        # 공통: 시간축과 각 시리즈 길이 맞춰주는 헬퍼
        def align_xy(x, y):
            n = min(len(x), len(y))
            return x[:n], y[:n]

        # --------- 그래프 1: Cell Voltage vs Time ----------
        plt.figure(figsize=(10, 5))
        for i in range(NUM_CELLS):
            if len(volt_logs[i]) == 0:
                continue  # 데이터 없으면 스킵
            tx, vy = align_xy(time_log, volt_logs[i])
            plt.plot(tx, vy, label=f"Cell {i+1} Voltage")
        plt.xlabel("Time [min]")
        plt.ylabel("Voltage [V]")
        plt.suptitle("Cell Voltage vs. Time", fontsize=16, fontweight='bold')
        plt.title("Experiment: Real Pack with Balancing", fontsize=10)
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()

        # --------- 그래프 2: Cell SOC vs Time ----------
        plt.figure(figsize=(10, 5))
        for i in range(NUM_CELLS):
            if len(soc_logs[i]) == 0:
                continue
            tx, sy = align_xy(time_log, soc_logs[i])
            plt.plot(tx, sy, "--", label=f"Cell {i+1} SOC")
        plt.xlabel("Time [min]")
        plt.ylabel("SOC (0.0 - 1.0)")
        plt.suptitle("Cell SOC vs. Time", fontsize=16, fontweight='bold')
        plt.title("Experiment: Real Pack with Balancing (Voltage-based SOC)", fontsize=10)
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.show()
        
        # ----- numpy 저장 -----
        np.save("time_log2.npy", np.array(time_log))
        np.save("volt_logs2.npy", np.array(volt_logs))  # shape: (4, N)
        np.save("soc_logs2.npy",  np.array(soc_logs))   # shape: (4, N)
        print("Saved numpy binary files: time_log2.npy, volt_logs2.npy, soc_logs2.npy")

        # ----- CSV 저장 -----
        # 전체 시리즈에서 공통으로 쓸 최소 길이 찾기
        min_len = len(time_log)
        for i in range(NUM_CELLS):
            min_len = min(min_len, len(volt_logs[i]), len(soc_logs[i]))

        t_aligned = time_log[:min_len]
        data = {"time_min": t_aligned}

        for i in range(NUM_CELLS):
            data[f"cell{i+1}_V"]   = volt_logs[i][:min_len]
            data[f"cell{i+1}_SOC"] = soc_logs[i][:min_len]

        df = pd.DataFrame(data)
        df.to_csv("bms_log2.csv", index=False)

        print("Saved CSV file: bms_log2.csv")