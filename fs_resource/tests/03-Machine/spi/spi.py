from machine import SPI
from machine import FPIOA
a = FPIOA()

a.help(14)
a.set_function(14,a.QSPI0_CS0)
a.help(14)

a.help(15)
a.set_function(15,a.QSPI0_CLK)
a.help(15)

a.help(16)
a.set_function(16,a.QSPI0_D0)
a.help(16)

a.help(17)
a.set_function(17,a.QSPI0_D1)
a.help(17)

spi=SPI(1,baudrate=5000000, polarity=0, phase=0, bits=8) # spi init clock 5MHz, polarity 0, phase 0, data bitwide 8bits

spi.write(bytes([0x66])) # enable gd25lq128 reset

spi.write(bytes([0x99])) # gd25lq128 reset

a=bytes([0x9f]) # send buff
b=bytearray(3) # receive buf
spi.write_readinto(a,b) # read gd25lq128 id 
print(b)  # bytearray(b'\xc8`\x18')

a=bytes([0x90,0,0,0]) # send buff
b=bytearray(2)  # receive buf
spi.write_readinto(a,b) # read gd25lq128 id 
print(b) # bytearray(b'\xc8\x17')
