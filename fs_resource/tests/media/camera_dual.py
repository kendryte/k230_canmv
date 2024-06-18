# Camera Example
#
# Note: You will need an SD card to run this example.
#
# You can start 2 camera preview.
import time, os, sys

from media.sensor import *
from media.display import *
from media.media import *

def camera_test():
    print("camera_test")

    sensor0 = Sensor(id = 0)
    sensor0.reset()
    # set chn0 output size, 960x540
    sensor0.set_framesize(width = 960, height = 540)
    # set chn0 out format
    sensor0.set_pixformat(Sensor.YUV420SP)
    # bind sensor chn0 to display layer video 1
    bind_info = sensor0.bind_info(x = 0, y = 0)
    Display.bind_layer(**bind_info, layer = Display.LAYER_VIDEO1)

    sensro1 = Sensor(id = 1)
    sensro1.reset()
    # set chn0 output size, 960x540
    sensro1.set_framesize(width = 960, height = 540)
    # set chn0 out format
    sensro1.set_pixformat(Sensor.YUV420SP)

    bind_info = sensro1.bind_info(x = 960, y = 540)
    Display.bind_layer(**bind_info, layer = Display.LAYER_VIDEO2)

    # use hdmi as display output
    Display.init(Display.LT9611, to_ide = True)
    # init media manager
    MediaManager.init()

    # multiple sensor only need one excute run()
    sensor0.run()

    try:
        while True:
            os.exitpoint()
            time.sleep(5)
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        print(f"Exception {e}")

    # multiple sensor all need excute stop()
    sensor0.stop()
    sensro1.stop()

    # deinit display
    Display.deinit()

    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # deinit media buffer
    MediaManager.deinit()

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    camera_test()
