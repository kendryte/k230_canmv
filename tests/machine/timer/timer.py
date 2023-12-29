from machine import Timer
import time

# 定义定时器回调函数
def on_timer(arg):                                                                                          
    print("timer callback: ", arg)

# 构造定时器对象，时间为1s
timer = Timer(1000000, on_timer, "callback param")
timer.start()
time.sleep(2)
