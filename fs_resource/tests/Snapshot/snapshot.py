# Emboss Snapshot Example
#
# Note: You will need an SD card to run this example.
#
# You can use your CanMV Cam to save modified image files.
import time, os

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

sensor = None

def camera_init():
    global sensor

    # construct a Sensor object with default configure
    sensor = Sensor()
    # sensor reset
    sensor.reset()
    # set hmirror
    # sensor.set_hmirror(False)
    # sensor vflip
    # sensor.set_vflip(False)

    # set chn0 output size, 1920x1080
    sensor.set_framesize(Sensor.FHD)
    # set chn0 output format
    sensor.set_pixformat(Sensor.YUV420SP)

    # set chn1 output format
    sensor.set_framesize(width= DISPLAY_WIDTH, height = DISPLAY_HEIGHT, chn = CAM_CHN_ID_1)
    sensor.set_pixformat(Sensor.RGB565, chn = CAM_CHN_ID_1)

    # init media manager
    MediaManager.init()
    # sensor start run
    sensor.run()

def camera_deinit():
    global sensor

    # sensor stop run
    sensor.stop()
    # sleep
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # release media buffer
    MediaManager.deinit()

def capture_picture():
    time.sleep(1)
    try:
        global sensor
        img = sensor.snapshot(chn = CAM_CHN_ID_1)
        img.save("/sdcard/snapshot.jpg")
        print("save image ok")
    except Exception as e:
        print("save image fail: ", e)

def main():
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    print("camera init")
    camera_init()
    print("camera capture")
    capture_picture()
    print("camera deinit")
    camera_deinit()

if __name__ == "__main__":
    main()
