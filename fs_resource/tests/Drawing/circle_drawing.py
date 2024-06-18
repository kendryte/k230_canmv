# Circle Drawing
#
# This example shows off drawing circles on the CanMV Cam.
import time, os, gc, sys, urandom

from media.display import *
from media.media import *

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

def draw():
    fps = time.clock()
    # create image for drawing
    img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)

    while True:
        fps.tick()
        img.clear()
        for i in range(10):
            x = (urandom.getrandbits(30) % (2*img.width())) - (img.width()//2)
            y = (urandom.getrandbits(30) % (2*img.height())) - (img.height()//2)
            radius = urandom.getrandbits(30) % (max(img.height(), img.width())//2)
            r = (urandom.getrandbits(30) % 127) + 128
            g = (urandom.getrandbits(30) % 127) + 128
            b = (urandom.getrandbits(30) % 127) + 128
            # If the first argument is a scaler then this method expects
            # to see x, y, and radius. Otherwise, it expects a (x,y,radius) tuple.
            img.draw_circle(x, y, radius, color = (r, g, b), thickness = 2, fill = False)
        # draw result to screen
        Display.show_image(img)

        time.sleep(1)
        os.exitpoint()

def main():
    os.exitpoint(os.EXITPOINT_ENABLE)

    try:
        print("camera init")
        # use hdmi as display output
        Display.init(Display.LT9611, to_ide = True)
        # init media manager
        MediaManager.init()
        print("draw")
        draw()
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        print(f"Exception {e}")
    finally:
        Display.deinit()
        MediaManager.deinit()

if __name__ == "__main__":
    main()
