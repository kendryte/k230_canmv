# 基础示例
#
# 欢迎使用CanMV IDE, 点击IDE左下角的绿色按钮开始执行脚本

from machine import GPIO

gpio = GPIO(8, GPIO.OUT, GPIO.PULL_UP, value=0)        #构造GPIO对象，gpio编号为8，设置为上拉输出低电平
value = gpio.value()                                   #获取gpio的值
print("value = %d" % value) 
gpio.value(1)                                          #设置gpio输出为高电平
value = gpio.value()
print("value = %d" % value)
