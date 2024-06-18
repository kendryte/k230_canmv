# Rotation Correction
#
# This example shows off how to use the rotation_corr() to both correct for
# perspective distortion and then to rotate the new corrected image in 3D
# space aftwards to handle movement.
import time, os, gc, sys

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# The image will be warped such that the following points become the new:
#
#   (0,   0)
#   (w-1, 0)
#   (w-1, h-1)
#   (0,   h-1)
#
# Try setting the points below to the corners of a quadrilateral
# (in clock-wise order) in the field-of-view. You can get points
# on the image by clicking and dragging on the frame buffer and
# recording the values shown in the histogram widget.

width= DETECT_WIDTH
height = DETECT_HEIGHT

TARGET_POINTS = [(0,   0),   # (x, y) CHANGE ME!
                 (w-1, 0),   # (x, y) CHANGE ME!
                 (w-1, h-1), # (x, y) CHANGE ME!
                 (0,   h-1)] # (x, y) CHANGE ME!

# Degrees per frame to rotation by...
X_ROTATION_DEGREE_RATE = 5
Y_ROTATION_DEGREE_RATE = 0.5
Z_ROTATION_DEGREE_RATE = 0
X_OFFSET = 0
Y_OFFSET = 0

ZOOM_AMOUNT = 1 # Lower zooms out - Higher zooms in.
FOV_WINDOW = 25 # Between 0 and 180. Represents the field-of-view of the scene
                # window when rotating the image in 3D space. When closer to
                # zero results in lines becoming straighter as the window
                # moves away from the image being rotated in 3D space. A large
                # value moves the window closer to the image in 3D space which
                # results in the more perspective distortion and sometimes
                # the image in 3D intersecting the scene window.

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
    x_rotation_counter = 0
    y_rotation_counter = 0
    z_rotation_counter = 0
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exitpoint()
            global sensor
            img = sensor.snapshot(chn = CAM_CHN_ID_1)
            img.rotation_corr(x_rotation = x_rotation_counter,
                                y_rotation = y_rotation_counter,
                                z_rotation = z_rotation_counter,
                                x_translation = X_OFFSET,
                                y_translation = Y_OFFSET,
                                zoom = ZOOM_AMOUNT,
                                fov = FOV_WINDOW,
                                corners = TARGET_POINTS)
            x_rotation_counter += X_ROTATION_DEGREE_RATE
            y_rotation_counter += Y_ROTATION_DEGREE_RATE
            z_rotation_counter += Z_ROTATION_DEGREE_RATE
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
