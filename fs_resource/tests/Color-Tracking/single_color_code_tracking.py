# Single Color Code Tracking Example
#
# This example shows off single color code tracking using the CanMV Cam.
#
# A color code is a blob composed of two or more colors. The example below will
# only track colored objects which have both the colors below in them.
import time, os, gc, sys, math

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# Color Tracking Thresholds (L Min, L Max, A Min, A Max, B Min, B Max)
# The below thresholds track in general red/green things. You may wish to tune them...
thresholds = [(30, 100, 15, 127, 15, 127), # generic_red_thresholds -> index is 0 so code == (1 << 0)
              (30, 100, -64, -8, -32, 32)] # generic_green_thresholds -> index is 1 so code == (1 << 1)
# Codes are or'ed together when "merge=True" for "find_blobs"

# Only blobs that with more pixels than "pixel_threshold" and more area than "area_threshold" are
# returned by "find_blobs" below. Change "pixels_threshold" and "area_threshold" if you change the
# camera resolution. "merge=True" must be set to merge overlapping color blobs for color codes.

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
    # sensor.set_pixformat(Sensor.RGB888, chn = CAM_CHN_ID_1)
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
    # create image for drawing
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)

    while True:
        fps.tick()
        try:
            os.exitpoint()
            draw_img.clear()

            global sensor
            img = sensor.snapshot(chn = CAM_CHN_ID_1)

            for blob in img.find_blobs(thresholds, pixels_threshold=100, area_threshold=100, merge=True):
                if blob.code() == 3: # r/g code == (1 << 1) | (1 << 0)
                    # These values depend on the blob not being circular - otherwise they will be shaky.
                    # if blob.elongation() > 0.5:
                    #     img.draw_edges(blob.min_corners(), color=(255,0,0))
                    #     img.draw_line(blob.major_axis_line(), color=(0,255,0))
                    #     img.draw_line(blob.minor_axis_line(), color=(0,0,255))
                    # These values are stable all the time.
                    draw_img.draw_rectangle([v*SCALE for v in blob.rect()])
                    draw_img.draw_cross(blob.cx()*SCALE, blob.cy()*SCALE)
                    # Note - the blob rotation is unique to 0-180 only.
                    draw_img.draw_keypoints([(blob.cx()*SCALE, blob.cy()*SCALE, int(math.degrees(blob.rotation())))], size=20)
            # draw result to screen
            Display.show_image(draw_img)

            img = None
            gc.collect()
            print(fps.fps())
        except KeyboardInterrupt as e:
            print("user stop: ", e)
            break
        except BaseException as e:
            sys.print_exception(e)
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
        sys.print_exception(e)
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
