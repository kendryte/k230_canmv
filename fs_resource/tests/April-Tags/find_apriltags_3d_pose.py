# AprilTags Example
#
# This example shows the power of the CanMV Cam to detect April Tags.

import time, math, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = 1920
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# Note! Unlike find_qrcodes the find_apriltags method does not need lens correction on the image to work.

# What's the difference between tag families? Well, for example, the TAG16H5 family is effectively
# a 4x4 square tag. So, this means it can be seen at a longer distance than a TAG36H11 tag which
# is a 6x6 square tag. However, the lower H value (H5 versus H11) means that the false positve
# rate for the 4x4 tag is much, much, much, higher than the 6x6 tag. So, unless you have a
# reason to use the other tags families just use TAG36H11 which is the default family.

# The AprilTags library outputs the pose information for tags. This is the x/y/z translation and
# x/y/z rotation. The x/y/z rotation is in radians and can be converted to degrees. As for
# translation the units are dimensionless and you must apply a conversion function.

# f_x is the x focal length of the camera. It should be equal to the lens focal length in mm
# divided by the x sensor size in mm times the number of pixels in the image.
# The below values are for the OV7725 camera with a 2.8 mm lens.

# f_y is the y focal length of the camera. It should be equal to the lens focal length in mm
# divided by the y sensor size in mm times the number of pixels in the image.
# The below values are for the OV7725 camera with a 2.8 mm lens.

# c_x is the image x center position in pixels.
# c_y is the image y center position in pixels.

f_x = (2.8 / 3.984) * DETECT_WIDTH # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * DETECT_HEIGHT # find_apriltags defaults to this if not set
c_x = DETECT_WIDTH * 0.5 # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = DETECT_HEIGHT * 0.5 # find_apriltags defaults to this if not set (the image.h * 0.5)

sensor = None

def degrees(radians):
    return (180 * radians) / math.pi

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
    sensor.set_framesize(width = DETECT_WIDTH, height = DETECT_HEIGHT, chn = CAM_CHN_ID_1)
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

    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)

    # release media buffer
    MediaManager.deinit()

def capture_picture():
    global sensor

    # create image for drawing
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)

    fps = time.clock()

    while True:
        fps.tick()
        try:
            os.exitpoint()
            draw_img.clear()

            yuv420_img = sensor.snapshot(chn = CAM_CHN_ID_1)
            dect_img = image.Image(yuv420_img.width(), yuv420_img.height(), image.GRAYSCALE, alloc=image.ALLOC_HEAP, data=yuv420_img)

            for tag in dect_img.find_apriltags(fx=f_x, fy=f_y, cx=c_x, cy=c_y): # defaults to TAG36H11
                draw_img.draw_rectangle([v*SCALE for v in tag.rect()], color=(255, 0, 0))
                draw_img.draw_cross(tag.cx()*SCALE, tag.cy()*SCALE, color=(0, 255, 0))
                print_args = (tag.x_translation(), tag.y_translation(), tag.z_translation(),
                    degrees(tag.x_rotation()), degrees(tag.y_rotation()), degrees(tag.z_rotation()))
                # Translation units are unknown. Rotation units are in degrees.
                print("Tx: %f, Ty %f, Tz %f, Rx %f, Ry %f, Rz %f" % print_args)

            print(fps.fps())

            # draw result to screen
            Display.show_image(draw_img)

            del dect_img
            gc.collect()
        except KeyboardInterrupt as e:
            print(f"user stop {e}")
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
