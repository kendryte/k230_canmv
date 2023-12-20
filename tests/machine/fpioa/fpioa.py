from machine import FPIOA
a = FPIOA()


a.help()
for i in range(63):
    a.help(i)

for i in range(221):
    a.get_Pin_num(i)

a.help(60)
a.set_function(60,a.GPIO60)
a.help(60)

a.set_function(60,set_sl=1,set_ie=0,set_oe=0,set_pd=1,set_pu=0,set_ds=6,set_st=0,set_di=1)
a.help(60)
try :
    print(a.set_function(60,set_msc=0))

except Exception as e:
        print("set error ", e)

a.help(60)



