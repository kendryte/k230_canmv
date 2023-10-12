from machine import ADC

adc = ADC(0,enable=True)
value = adc.value()
print("value = %d" % value)
adc.deinit()
