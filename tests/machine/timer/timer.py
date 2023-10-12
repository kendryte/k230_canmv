from machine import Timer


def on_timer(arg):
    print("time up: %d" % arg)

tim = Timer(mode=Timer.MODE_ONE_SHOT,period=3, unit=Timer.UNIT_S, callback=on_timer, arg=1, start=True)

