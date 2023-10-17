from machine import I2C

i2c4=machine.I2C(4)

i2c4.scan()

i2c4.writeto_mem(0x3b,0xff,bytes([0x80]),mem_size=8)
#value =0x17
i2c4.readfrom_mem(0x3b,0x00,1,mem_size=8)   
#value =0x2        
i2c4.readfrom_mem(0x3b,0x01,1,mem_size=8)

i2c4.writeto(0x3b,bytes([0xff,0x80]),True)
i2c4.writeto(0x3b,bytes([0x00]),True)
#value =0x17
i2c4.readfrom(0x3b,1)
i2c4.writeto(0x3b,bytes([0x01]),True)
#value =0x2
i2c4.readfrom(0x3b,1)

i2c4.writeto_mem(0x3b,0xff,bytes([0x80]),mem_size=8)
a=bytearray(1)
i2c4.readfrom_mem_into(0x3b,0x0,a,mem_size=8)
#value =0x17
print(a)

i2c4.writeto(0x3b,bytes([0xff,0x80]),True)
i2c4.writeto(0x3b,bytes([0x00]),True)
b=bytearray(1)
i2c4.readfrom_into(0x3b,b)
#value =0x17
print(b)