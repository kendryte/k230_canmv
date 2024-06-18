# Black Grayscale Line Following Example
#
# Making a line following robot requires a lot of effort. This example script
# shows how to do the machine vision part of the line following robot. You
# can use the output from this script to drive a differential drive robot to
# follow a line. This script just generates a single turn value that tells
# your robot to go left or right.
#
# For this script to work properly you should point the camera at a line at a
# 45 or so degree angle. Please make sure that only the line is within the
# camera's field of view.
import time, os, gc, sys, math

from media.sensor import *
from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# Tracks a black line. Use [(128, 255)] for a tracking a white line.
GRAYSCALE_THRESHOLD = [(0, 64)]

# Each roi is (x, y, w, h). The line detection algorithm will try to find the
# centroid of the largest blob in each roi. The x position of the centroids
# will then be averaged with different weights where the most weight is assigned
# to the roi near the bottom of the image and less to the next roi and so on.
ROIS = [ # [ROI, weight]
        (0, 100, 160, 20, 0.7), # You'll need to tweak the weights for your app
        (0,  50, 160, 20, 0.3), # depending on how your robot is setup.
        (0,   0, 160, 20, 0.1)
       ]

# Compute the weight divisor (we're computing this so you don't have to make weights add to 1).
weight_sum = 0
for r in ROIS: weight_sum += r[4] # r[4] is the roi weight.

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
    sensor.set_pixformat(Sensor.RGB888, chn = CAM_CHN_ID_1)

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
            rgb888_img = sensor.snapshot(chn = CAM_CHN_ID_1)
            img = rgb888_img.to_grayscale()

            centroid_sum = 0
            for r in ROIS:
                blobs = img.find_blobs(GRAYSCALE_THRESHOLD, roi=r[0:4], merge=True) # r[0:4] is roi tuple.

                if blobs:
                    # Find the blob with the most pixels.
                    largest_blob = max(blobs, key=lambda b: b.pixels())

                    # Draw a rect around the blob.
                    draw_img.draw_rectangle([v*SCALE for v in largest_blob.rect()])
                    draw_img.draw_cross(largest_blob.cx()*SCALE, largest_blob.cy()*SCALE)

                    centroid_sum += largest_blob.cx() * r[4] # r[4] is the roi weight.

            center_pos = (centroid_sum / weight_sum) # Determine center of line.

            # Convert the center_pos to a deflection angle. We're using a non-linear
            # operation so that the response gets stronger the farther off the line we
            # are. Non-linear operations are good to use on the output of algorithms
            # like this to cause a response "trigger".
            deflection_angle = 0

            # The 80 is from half the X res, the 60 is from half the Y res. The
            # equation below is just computing the angle of a triangle where the
            # opposite side of the triangle is the deviation of the center position
            # from the center and the adjacent side is half the Y res. This limits
            # the angle output to around -45 to 45. (It's not quite -45 and 45).
            deflection_angle = -math.atan((center_pos-80)/60)

            # Convert angle in radians to degrees.
            deflection_angle = math.degrees(deflection_angle)

            # Now you have an angle telling you how much to turn the robot by which
            # incorporates the part of the line nearest to the robot and parts of
            # the line farther away from the robot for a better prediction.
            print("Turn Angle: %f" % deflection_angle)

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
