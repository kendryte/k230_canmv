# Cartoon Filter
#
# This example shows off a simple cartoon filter on images. The cartoon
# filter works by joining similar pixel areas of an image and replacing
# the pixels in those areas with the area mean.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE


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
    # bind sensor chn0 to display layer video 1
    # bind_info = sensor.bind_info()
    # Display.bind_layer(**bind_info, layer = Display.LAYER_VIDEO1)

    # set chn1 output format
    sensor.set_framesize(width= DETECT_WIDTH, height = DETECT_HEIGHT, chn = CAM_CHN_ID_1)
    sensor.set_pixformat(Sensor.RGB565, chn = CAM_CHN_ID_1)

    # use hdmi as display output
    Display.init(Display.LT9611, to_ide = True)
    # init media manager
    MediaManager.init()
    # sensor start run
    sensor.run()

def camera_deinit():
    global sensor

    # sensor stop run
    sensor.stop()
    # deinit display
    Display.deinit()
    # sleep
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # release media buffer
    MediaManager.deinit()

def capture_picture():
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exitpoint()

            global sensor
            img = sensor.snapshot(chn = CAM_CHN_ID_1)

            # seed_threshold controls the maximum area growth of a colored
            # region. Making this larger will merge more pixels.

            # floating_threshold controls the maximum pixel-to-pixel difference
            # when growing a region. Settings this very high will quickly combine
            # all pixels in the image. You should keep this small.

            # cartoon() will grow regions while both thresholds are statisfied...

            img.cartoon(seed_threshold=0.05, floating_thresholds=0.05)
            # draw result to screen
            Display.show_image(img)
            img = None
            gc.collect()
            print(fps.fps())
        except KeyboardInterrupt as e:
            print("user stop: ", e)
            break
        except BaseException as e:
            print(f"Exception {e}")
            break

def main():
    os.exitpoint(os.EXITPOINT_ENABLE)
    camera_is_init = False
    try:
        print("camera init")
        camera_init()
        camera_is_init = True
        print("camera capture")
        capture_picture()
    except Exception as e:
        print(f"Exception {e}")
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
