from machine import PWM

# channel 0 output freq 1kHz duty 50%, enable
pwm0 = PWM(0, 1000, 50, enable = True)
