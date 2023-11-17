# 基础示例
#
# 欢迎使用CanMV IDE, 点击IDE左下角的绿色按钮开始执行脚本

from machine import Timer
def on_timer(arg):                                                                                          #定义定时器回调函数
    print("time up: %d" % arg)

tim = Timer(mode=Timer.MODE_ONE_SHOT,period=3, unit=Timer.UNIT_S, callback=on_timer, arg=1, start=True)     #构造定时器对象，时间为3s，默认开启定时器

