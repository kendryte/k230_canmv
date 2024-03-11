from machine import Pin
from machine import FPIOA

# 实例化FPIOA
fpioa = FPIOA()
# 设置Pin2为GPIO2
fpioa.set_function(2, FPIOA.GPIO2)

# 实例化Pin2为输出
pin = Pin(2, Pin.OUT, pull=Pin.PULL_NONE, drive=7)
# 设置输出为高
pin.value(1)
# pin.on()
# pin.high()
# 设置输出为低
pin.value(0)
# pin.off()
# pin.low()
# 初始化Pin2为输入
pin.init(Pin.IN, pull=Pin.PULL_UP, drive=7)
# 获取输入
print(pin.value())
# 设置模式
pin.mode(Pin.IN)
# 获取模式
print(pin.mode())
# 设置上下拉
pin.pull(Pin.PULL_NONE)
# 获取上下拉
print(pin.pull())
# 设置驱动能力
pin.drive(7)
# 获取驱动能力
print(pin.drive())
