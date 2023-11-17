# 基础示例
#
# 欢迎使用CanMV IDE, 点击IDE左下角的绿色按钮开始执行脚本

import time
from machine import WDT

wdt1 = WDT(1,3)                         #构造wdt对象，/dev/watchdog1,timeout为3s
print('into', wdt1)
time.sleep(2)                           #延时2s
print(time.ticks_ms())                  
## 1.test wdt feed
wdt1.feed()                             #喂狗操作
time.sleep(2)                           #延时2s
print(time.ticks_ms())
## 2.test wdt stop
wdt1.stop()                             #停止喂狗
