# Template Matching Example - Normalized Cross Correlation (NCC)
#
# This example shows off how to use the NCC feature of your CanMV Cam to match
# image patches to parts of an image... expect for extremely controlled enviorments
# NCC is not all to useful.
#
# WARNING: NCC supports needs to be reworked! As of right now this feature needs
# a lot of work to be made into somethin useful. This script will reamin to show
# that the functionality exists, but, in its current state is inadequate.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

from image import SEARCH_EX, SEARCH_DS

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
    # create image for drawing
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)

    # Load template.
    # Template should be a small (eg. 32x32 pixels) grayscale image.
    # template = image.Image("/sd/template.bmp")
    # template.to_grayscale()
    template = image.Image(32, 32, image.GRAYSCALE)
    template.draw_circle(16,16,16,color=(255,255,255),fill=True)
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exitpoint()
            draw_img.clear()

            global sensor
            yuv420_img = sensor.snapshot(chn = CAM_CHN_ID_1)
            img = image.Image(yuv420_img.width(), yuv420_img.height(), image.GRAYSCALE, alloc=image.ALLOC_HEAP, data=yuv420_img)

            # find_template(template, threshold, [roi, step, search])
            # ROI: The region of interest tuple (x, y, w, h).
            # Step: The loop step used (y+=step, x+=step) use a bigger step to make it faster.
            # Search is either image.SEARCH_EX for exhaustive search or image.SEARCH_DS for diamond search
            #
            # Note1: ROI has to be smaller than the image and bigger than the template.
            # Note2: In diamond search, step and ROI are both ignored.
            r = img.find_template(template, 0.50, step=4, search=SEARCH_EX) #, roi=(10, 0, 60, 60))
            if r:
                draw_img.draw_rectangle([v*SCALE for v in r],color=(255,0,0))

            # draw result to screen
            Display.show_image(draw_img)

            del img
            gc.collect()
            print(fps.fps())
        except KeyboardInterrupt as e:
            print("user stop: ", e)
            break
        except BaseException as e:
            print(f"Exception {e}")
            break
    del draw_img
    gc.collect()

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
