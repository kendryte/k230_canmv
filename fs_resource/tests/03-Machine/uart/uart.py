from machine import UART
from machine import FPIOA

# 实例化FPIOA
fpioa = FPIOA()
# 设置PIN60为PWM通道0
fpioa.set_function(5, fpioa.UART2_TXD)
fpioa.set_function(6, fpioa.UART2_RXD)
# UART2: baudrate 115200, 8bits, parity none, one stopbits
uart = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
# UART write
r = uart.write("UART test")
print(r)
# UART read
r = uart.read()
print(r)
# UART readline
r = uart.readline()
print(r)
# UART readinto
b = bytearray(8)
r = uart.readinto(b)
print(r)
