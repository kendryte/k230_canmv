# Data Matrices Example
#
# This example shows off how easy it is to detect data matrices.
import time, math, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DETECT_WIDTH = 640
DETECT_HEIGHT = 480

sensor = None

try:
    # construct a Sensor object with default configure
    sensor = Sensor(width = DETECT_WIDTH, height = DETECT_HEIGHT)
    # sensor reset
    sensor.reset()
    # set hmirror
    # sensor.set_hmirror(False)
    # sensor vflip
    # sensor.set_vflip(False)
    # set chn0 output size
    sensor.set_framesize(width = DETECT_WIDTH, height = DETECT_HEIGHT)
    # set chn0 output format
    sensor.set_pixformat(Sensor.GRAYSCALE)

    # use hdmi as display output, set to VGA
    # Display.init(Display.LT9611, width = 640, height = 480, to_ide = True)

    # use hdmi as display output, set to 1080P
    # Display.init(Display.LT9611, width = 1920, height = 1080, to_ide = True)

    # use lcd as display output
    # Display.init(Display.ST7701, to_ide = True)

    # use IDE as output
    Display.init(Display.VIRT, width = DETECT_WIDTH, height = DETECT_HEIGHT, fps = 100)

    # init media manager
    MediaManager.init()
    # sensor start run
    sensor.run()

    fps = time.clock()

    while True:
        fps.tick()

        # check if should exit.
        os.exitpoint()
        img = sensor.snapshot()

        for matrix in img.find_datamatrices():
            img.draw_rectangle([v for v in matrix.rect()], color=(255, 0, 0))
            print_args = (matrix.rows(), matrix.columns(), matrix.payload(), (180 * matrix.rotation()) / math.pi, fps.fps())
            print("Matrix [%d:%d], Payload \"%s\", rotation %f (degrees), FPS %f" % print_args)

        # draw result to screen
        Display.show_image(img)
        gc.collect()

        print(fps.fps())
except KeyboardInterrupt as e:
    print(f"user stop")
except BaseException as e:
    print(f"Exception '{e}'")
finally:
    # sensor stop run
    if isinstance(sensor, Sensor):
        sensor.stop()
    # deinit display
    Display.deinit()

    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)

    # release media buffer
    MediaManager.deinit()
