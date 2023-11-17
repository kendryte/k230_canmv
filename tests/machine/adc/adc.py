# 基础示例
#
# 欢迎使用CanMV IDE, 点击IDE左下角的绿色按钮开始执行脚本

from machine import ADC

adc = ADC(0,enable=True)                      #构造adc对象，通道0默认开启
value = adc.value()                           #获取通道0的数值
print("value = %d" % value)
adc.deinit()                                  #注销adc对象
