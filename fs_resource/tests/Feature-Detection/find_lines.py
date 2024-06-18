# Find Lines Example
#
# This example shows off how to find lines in the image. For each line object
# found in the image a line object is returned which includes the line's rotation.

# Note: Line detection is done by using the Hough Transform:
# http://en.wikipedia.org/wiki/Hough_transform
# Please read about it above for more information on what `theta` and `rho` are.

# find_lines() finds infinite length lines. Use find_line_segments() to find non-infinite lines.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# All line objects have a `theta()` method to get their rotation angle in degrees.
# You can filter lines based on their rotation angle.

min_degree = 0
max_degree = 179

# All lines also have `x1()`, `y1()`, `x2()`, and `y2()` methods to get their end-points
# and a `line()` method to get all the above as one 4 value tuple for `draw_line()`.

# About negative rho values:
#
# A [theta+0:-rho] tuple is the same as [theta+180:+rho].
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
    bind_info = sensor.bind_info()
    Display.bind_layer(**bind_info, layer = Display.LAYER_VIDEO1)

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

            # `threshold` controls how many lines in the image are found. Only lines with
            # edge difference magnitude sums greater than `threshold` are detected...

            # More about `threshold` - each pixel in the image contributes a magnitude value
            # to a line. The sum of all contributions is the magintude for that line. Then
            # when lines are merged their magnitudes are added togheter. Note that `threshold`
            # filters out lines with low magnitudes before merging. To see the magnitude of
            # un-merged lines set `theta_margin` and `rho_margin` to 0...

            # `theta_margin` and `rho_margin` control merging similar lines. If two lines
            # theta and rho value differences are less than the margins then they are merged.

            for l in img.find_lines(threshold = 1000, theta_margin = 25, rho_margin = 25):
                if (min_degree <= l.theta()) and (l.theta() <= max_degree):
                    img.draw_line([v*SCALE for v in l.line()], color = (255, 0, 0))
                    print(l)

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
