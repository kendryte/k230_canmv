from machine import I2C

i2c4=I2C(4)     # init i2c4

a=i2c4.scan()     #scan i2c slave
print(a)

i2c4.writeto_mem(0x3b,0xff,bytes([0x80]),mem_size=8)    # write hdmi page address(0x80)
i2c4.readfrom_mem(0x3b,0x00,1,mem_size=8)   # read hdmi id0 ,value =0x17     
i2c4.readfrom_mem(0x3b,0x01,1,mem_size=8)    # read hdmi id1 ,value =0x2 

i2c4.writeto(0x3b,bytes([0xff,0x80]),True)  # write hdmi page address(0x80)
i2c4.writeto(0x3b,bytes([0x00]),True)   #send the address0 of being readed
i2c4.readfrom(0x3b,1)   #read hdmi id0 ,value =0x17 
i2c4.writeto(0x3b,bytes([0x01]),True)   #send the address1 of being readed
i2c4.readfrom(0x3b,1)   #read hdmi id0 ,value =0x17 

i2c4.writeto_mem(0x3b,0xff,bytes([0x80]),mem_size=8)    # write hdmi page address(0x80)
a=bytearray(1)
i2c4.readfrom_mem_into(0x3b,0x0,a,mem_size=8)   # read hdmi id0 into a ,value =0x17     
print(a)    #printf a,value =0x17 

i2c4.writeto(0x3b,bytes([0xff,0x80]),True)  # write hdmi page address(0x80)
i2c4.writeto(0x3b,bytes([0x00]),True)   #send the address0 of being readed
b=bytearray(1)
i2c4.readfrom_into(0x3b,b)   #read hdmi id0 into b ,value =0x17
print(b)    #printf a,value =0x17 