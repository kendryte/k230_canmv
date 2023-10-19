from machine import FFT
import array
import math
from ulab import numpy as np
PI = 3.14159265358979323846264338327950288419716939937510

rx = []
def input_data():
    for i in range(64):
        data0 = 10 * math.cos(2 * PI * i / 64)
        data1  = 20 * math.cos(2 * 2 * PI * i / 64)
        data2  = 30 * math.cos(3 * 2 * PI * i / 64)
        data3  = 0.2 * math.cos(4 * 2 * PI * i / 64)
        data4  = 1000 * math.cos(5 * 2 * PI * i / 64)
        rx.append((int(data0 + data1 + data2 + data3 + data4))) 
input_data()
print(rx)
data = np.array(rx,dtype=np.uint16)
print(data)
fft1 = FFT(data, 64, 0x555)
res = fft1.run()
print(res)

res = fft1.amplitude(res)
print(res)

res = fft1.freq(64,38400)
print(res)


