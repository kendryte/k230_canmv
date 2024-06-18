# Draw Image Testing script with bounce
#
# Exercise draw image with many different values for testing
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

BOUNCE = True
RESCALE = True

SMALL_IMAGE_SCALE = 3

CYCLE_FORMATS = True
CYCLE_MASK = True

# Used when CYCLE_FORMATS or CYCLE_MASK is true
value_mixer_init = 0

# Location of small image
x_init = 100
y_init = 50

# Bounce direction
xd_init = 1
yd_init = 1

# Small image scaling
rescale_init = 1.0
rd_init = 0.1
max_rescale = 5
min_rescale = 0.2

# Boundary to bounce within
xmin = -DISPLAY_WIDTH / SMALL_IMAGE_SCALE - 8
ymin = -DISPLAY_HEIGHT / SMALL_IMAGE_SCALE - 8
xmax = DISPLAY_WIDTH + 8
ymax = DISPLAY_HEIGHT + 8

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
    sensor.set_pixformat(Sensor.RGB565)

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

def draw():
    value_mixer = value_mixer_init
    x = x_init
    y = y_init
    xd = xd_init
    yd = yd_init
    rescale = rescale_init
    rd = rd_init
    fps = time.clock()
    while True:
        fps.tick()
        status = ""
        value_mixer = value_mixer + 1
        try:
            os.exitpoint()

            global sensor
            img = sensor.snapshot()

            small_img = img.mean_pooled(SMALL_IMAGE_SCALE, SMALL_IMAGE_SCALE)
            status = 'rgb565 '
            if CYCLE_FORMATS:
                image_format = (value_mixer >> 8) & 3
                # To test combining different formats
                if (image_format==1): small_img = small_img.to_bitmap(); status = 'bitmap '
                if (image_format==2): small_img = small_img.to_grayscale(); status = 'grayscale '
                if (image_format==3): small_img = small_img.to_rgb565(); status = 'rgb565 '

            # update small image location
            if BOUNCE:
                x = x + xd
                if (x<xmin or x>xmax):
                    xd = -xd

                y = y + yd
                if (y<ymin or y>ymax):
                    yd = -yd

            # Update small image scale
            if RESCALE:
                rescale = rescale + rd
                if (rescale<min_rescale or rescale>max_rescale):
                    rd = -rd

            # Find the center of the image
            scaled_width = int(small_img.width() * abs(rescale))
            scaled_height= int(small_img.height() * abs(rescale))

            apply_mask = CYCLE_MASK and ((value_mixer >> 9) & 1)
            if apply_mask:
                img.draw_image(small_img, int(x), int(y), mask=small_img.to_bitmap(), x_scale=rescale, y_scale=rescale, alpha=240)
                status += 'alpha:240 '
                status += '+mask '
            else:
                img.draw_image(small_img, int(x), int(y), x_scale=rescale, y_scale=rescale, alpha=128)
                status += 'alpha:128 '

            img.draw_string_advanced(8, 0, 32, status)

            # draw result to screen
            Display.show_image(img)

            img = None
            del small_img
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
        print("draw")
        draw()
    except Exception as e:
        print(f"Exception {e}")
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
