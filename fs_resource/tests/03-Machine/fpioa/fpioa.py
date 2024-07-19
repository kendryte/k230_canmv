from machine import FPIOA

# 实例化FPIOA
fpioa = FPIOA()
# 打印所有引脚配置
fpioa.help()
# 打印指定引脚详细配置
fpioa.help(0)
# 打印指定功能所有可用的配置引脚
fpioa.help(FPIOA.IIC0_SDA, func=True)
# 设置Pin0为GPIO0
fpioa.set_function(0, FPIOA.GPIO0)
# 设置Pin2为GPIO2, 同时配置其它项
fpioa.set_function(2, FPIOA.GPIO2, ie=1, oe=1, pu=0, pd=0, st=1, sl=0, ds=7)
# 获取指定功能当前所在的引脚
fpioa.get_pin_num(FPIOA.UART0_TXD)
# 获取指定引脚当前功能
fpioa.get_pin_func(0)
