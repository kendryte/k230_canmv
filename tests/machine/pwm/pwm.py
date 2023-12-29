from machine import PWM
from machine import FPIOA

# 实例化FPIOA
fpioa = FPIOA()
# 设置PIN60为PWM通道0
fpioa.set_function(60, fpioa.PWM0)
# 实例化PWM通道0，频率为1000Hz，占空比为50%，默认使能输出
pwm0 = PWM(0, 1000, 50, enable = True)
# 关闭通道0输出
pwm0.enable(0)
# 调整通道0频率为2000Hz
pwm0.freq(2000)
# 调整通道0占空比为40%
pwm0.duty(40)
# 打开通道0输出
pwm0.enable(1)
