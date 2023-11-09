from machine import SPI

spi=machine.SPI(1,baudrate=5000000, polarity=0, phase=0, bits=8) # spi init clock 5MHz, polarity 0, phase 0, data bitwide 8bits

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