from machine import TOUCH

# 实例化TOUCH设备0
tp = TOUCH(0)
# 获取TOUCH数据
p = tp.read()
print(p)
