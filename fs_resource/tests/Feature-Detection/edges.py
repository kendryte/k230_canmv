# Edge detection with Canny:
#
# This example demonstrates the Canny edge detector.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *


DETECT_WIDTH = ALIGN_UP(640, 16)
DETECT_HEIGHT = 480

sensor = None

def camera_init():
    global sensor
    # construct a Sensor object with default configure
    sensor = Sensor(width=DETECT_WIDTH,height=DETECT_HEIGHT)
    # sensor reset
    sensor.reset()
    # set hmirror
    # sensor.set_hmirror(False)
    # sensor vflip
    # sensor.set_vflip(False)

    # set chn0 output size
    sensor.set_framesize(width=DETECT_WIDTH,height=DETECT_HEIGHT)
    # set chn0 output format
    sensor.set_pixformat(Sensor.GRAYSCALE)

    # use IDE as display output
    Display.init(Display.VIRT, width= DETECT_WIDTH, height = DETECT_HEIGHT,fps=100,to_ide = True)
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
            dect_img = sensor.snapshot()
            # Use Canny edge detector
            dect_img.find_edges(image.EDGE_CANNY, threshold=(50, 80))
            # Faster simpler edge detection
            #dect_img.find_edges(image.EDGE_SIMPLE, threshold=(100, 255))

            # draw result to screen
            Display.show_image(dect_img)

            del dect_img
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
