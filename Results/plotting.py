import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Raw data
data = """
Lane Count,Matrix Size,Expected Cycles,Performance (FLOP/cycle),Utilization (%)
16,4x4,692,0.185,0.58
16,8x8,921,1.112,3.47
16,16x16,2589,3.164,9.88
16,32x32,8028,8.163,25.51
16,64x64,33847,15.490,48.40
16,128x128,172772,24.277,75.86
8,4x4,690,0.186,1.16
8,8x8,923,1.109,6.93
8,16x16,2622,3.124,19.53
8,32x32,8722,7.514,46.96
8,64x64,43273,12.116,75.72
8,128x128,276181,15.187,94.92
4,4x4,702,0.182,2.28
4,8x8,940,1.089,13.62
4,16x16,2770,2.957,36.97
4,32x32,11444,5.727,71.58
4,64x64,70126,7.476,93.45
4,128x128,532649,7.874,98.43
2,4x4,549,0.233,5.83
2,8x8,858,1.193,29.84
2,16x16,3199,2.561,64.02
2,32x32,18374,3.567,89.17
2,64x64,139599,3.756,93.89
2,128x128,1064261,3.94,98.53
"""

# Load into pandas DataFrame
from io import StringIO
df = pd.read_csv(StringIO(data))

# Convert Matrix Size to numeric (e.g., 4x4 -> 4, 8x8 -> 8 ...)
df["Matrix Size Num"] = df["Matrix Size"].apply(lambda x: int(x.split("x")[0]))

# Plot styles
sns.set(style="whitegrid", font_scale=1.2, palette="tab10")

# --- Plot 1: Performance vs Matrix Size ---
plt.figure(figsize=(10,6))
sns.lineplot(data=df, x="Matrix Size Num", y="Performance (FLOP/cycle)", hue="Lane Count", marker="o")
plt.xscale("log", base=2)
plt.title("Performance vs Matrix Size")
plt.xlabel("Matrix Size (N x N)")
plt.ylabel("Performance (FLOP/cycle)")
plt.legend(title="Lane Count")
plt.show()

# --- Plot 2: Utilization vs Matrix Size ---
plt.figure(figsize=(10,6))
sns.lineplot(data=df, x="Matrix Size Num", y="Utilization (%)", hue="Lane Count", marker="o")
plt.xscale("log", base=2)
plt.title("Utilization vs Matrix Size")
plt.xlabel("Matrix Size (N x N)")
plt.ylabel("Utilization (%)")
plt.legend(title="Lane Count")
plt.show()

# --- Plot 3: Expected Cycles vs Matrix Size ---
plt.figure(figsize=(10,6))
sns.lineplot(data=df, x="Matrix Size Num", y="Expected Cycles", hue="Lane Count", marker="o")
plt.xscale("log", base=2)
plt.yscale("log")   # cycles grow exponentially, so log-scale
plt.title("Expected Cycles vs Matrix Size")
plt.xlabel("Matrix Size (N x N)")
plt.ylabel("Expected Cycles (log scale)")
plt.legend(title="Lane Count")
plt.show()
