# 基础示例
#
# 欢迎使用CanMV IDE, 点击IDE左下角的绿色按钮开始执行脚本

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
input_data()                                                            #初始化需要进行FFT的数据，列表类型
print(rx)
data = np.array(rx,dtype=np.uint16)                                     #把列表数据转换成数组
print(data)
fft1 = FFT(data, 64, 0x555)                                             #创建一个FFT对象,运算点数为64，偏移是0x555
res = fft1.run()                                                        #获取FFT转换后的数据
print(res)

res = fft1.amplitude(res)                                               #获取各个频率点的幅值                                      
print(res)

res = fft1.freq(64,38400)                                               #获取所有频率点的频率值  
print(res)


