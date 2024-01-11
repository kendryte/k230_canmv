from machine import UART
from machine import FPIOA
import time

# 实例化FPIOA
from machine import FPIOA
fpioa = FPIOA()
#pin3 设置为串口1发送管脚
fpioa.set_function(3, fpioa.UART1_TXD)
#设置pin4为串口1接收管脚
fpioa.set_function(4, fpioa.UART1_RXD)
#使能pin3的输入输出功能
fpioa.set_function(3,set_ie=1,set_oe=1)
#使能pin3的输入输出功能
fpioa.set_function(4,set_ie=1,set_oe=1)


#fpioa.set_function(5, fpioa.UART2_TXD)
#fpioa.set_function(6, fpioa.UART2_RXD)


#UART: baudrate 115200, 8bits, parity none, one stopbits
uart = UART(UART.UART1, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
#打印串口配置
print(uart)
# UART write
r = uart.write("UART test wwwwwwwwwwwww")
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
i=0
while True:
    #print( "xxx %d" % (0) )
    uart.write("i={0}".format(0))
    i=i+1
    #print(uart.read())
    a=uart.read()
    if len(a) > 1 :
        print(a,len(a))
    time.sleep(0.1)

