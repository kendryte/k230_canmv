# Robust Linear Regression Example
#
# This example shows off how to use the get_regression() method on your CanMV Cam
# to get the linear regression of a ROI. Using this method you can easily build
# a robot which can track lines which all point in the same general direction
# but are not actually connected. Use find_blobs() on lines that are nicely
# connected for better filtering options and control.
#
# We're using the robust=True argument for get_regression() in this script which
# computes the linear regression using a much more robust algorithm... but potentially
# much slower. The robust algorithm runs in O(N^2) time on the image. So, YOU NEED
# TO LIMIT THE NUMBER OF PIXELS the robust algorithm works on or it can actually
# take seconds for the algorithm to give you a result... THRESHOLD VERY CAREFULLY!
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

THRESHOLD = (0, 100) # Grayscale threshold for dark things...
BINARY_VISIBLE = True # Does binary first so you can see what the linear regression
                      # is being run on... might lower FPS though.

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
    sensor.set_pixformat(Sensor.YUV420SP, chn = CAM_CHN_ID_1)

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
            yuv420_img = sensor.snapshot(chn = CAM_CHN_ID_1)
            img = image.Image(yuv420_img.width(), yuv420_img.height(), image.GRAYSCALE, alloc=image.ALLOC_HEAP, data=yuv420_img)

            img = img.binary([THRESHOLD]) if BINARY_VISIBLE else img
            # Returns a line object similar to line objects returned by find_lines() and
            # find_line_segments(). You have x1(), y1(), x2(), y2(), length(),
            # theta() (rotation in degrees), rho(), and magnitude().
            #
            # magnitude() represents how well the linear regression worked. It goes from
            # (0, INF] where 0 is returned for a circle. The more linear the
            # scene is the higher the magnitude.
            line = img.get_regression([(255,255) if BINARY_VISIBLE else THRESHOLD], robust = True)
            if (line): img.draw_line(line.line(), color = 127)
            print("FPS %f, mag = %s" % (fps.fps(), str(line.magnitude()) if (line) else "N/A"))

            # draw result to screen
            Display.show_image(img)

            del img
            gc.collect()
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
