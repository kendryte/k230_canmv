import time
from machine import WDT

# 实例化wdt1，timeout为3s
wdt1 = WDT(1,3)
time.sleep(2)
# 喂狗操作
wdt1.feed()
time.sleep(2)
