import time
from machine import WDT

wdt1 = WDT(1,3)
print('into', wdt1)
time.sleep(2)
print(time.ticks_ms())
## 1.test wdt feed
wdt1.feed()
time.sleep(2)
print(time.ticks_ms())
## 2.test wdt stop
wdt1.stop()
