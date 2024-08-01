from machine import TOUCH

# 实例化TOUCH设备0
tp = TOUCH(0)
# 获取TOUCH数据

while True:
    point = tp.read(1)
    if len(point):
        print(point)
        break
