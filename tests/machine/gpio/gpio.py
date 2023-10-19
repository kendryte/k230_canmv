from machine import GPIO

gpio = GPIO(8, GPIO.OUT, GPIO.PULL_UP, value=0)
value = gpio.value()
print("value = %d" % value)
gpio.value(1)
value = gpio.value()
print("value = %d" % value)

def on_timer(arg):
    print("time up: %d" % arg)

gpio.irq(on_timer,3)
