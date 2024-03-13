from machine import RTC

# 实例化RTC
rtc = RTC()
# 获取当前时间
print(rtc.datetime())
# 设置当前时间
rtc.init((2024,2,28,2,23,59,0,0))
