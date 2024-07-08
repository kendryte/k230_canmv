from machine import ADC

# 实例化ADC通道0
adc = ADC(0)
# 获取ADC通道0采样值
print(adc.read_u16())
# 获取ADC通道0电压值
print(adc.read_uv(), "uV")
