# Automatic RGB565 Color Tracking Example
#
# This example shows off single color automatic RGB565 color tracking using the CanMV Cam.
import time, os, gc, sys, math

from media.sensor import *
from media.display import *
from media.media import *

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

    # Capture the color thresholds for whatever was in the center of the image.
    r = [(DETECT_WIDTH//2)-(50//2), (DETECT_HEIGHT//2)-(50//2), 50, 50] # 50x50 center of QVGA.
    threshold = [50, 50, 0, 0, 0, 0] # Middle L, A, B values.
    frame_count = 0
    while True:
        fps.tick()
        try:
            os.exitpoint()
            draw_img.clear()

            global sensor
            img = sensor.snapshot(chn = CAM_CHN_ID_1)

            if frame_count < 60:
                if frame_count == 0:
                    print("Letting auto algorithms run. Don't put anything in front of the camera!")
                    print("Auto algorithms done. Hold the object you want to track in front of the camera in the box.")
                    print("MAKE SURE THE COLOR OF THE OBJECT YOU WANT TO TRACK IS FULLY ENCLOSED BY THE BOX!")
                draw_img.draw_rectangle([v*SCALE for v in r])
                frame_count = frame_count + 1
            elif frame_count < 120:
                if frame_count == 60:
                    print("Learning thresholds...")
                elif frame_count == 119:
                    print("Thresholds learned...")
                    print("Tracking colors...")
                hist = img.get_histogram(roi=r)
                lo = hist.get_percentile(0.01) # Get the CDF of the histogram at the 1% range (ADJUST AS NECESSARY)!
                hi = hist.get_percentile(0.99) # Get the CDF of the histogram at the 99% range (ADJUST AS NECESSARY)!
                # Average in percentile values.
                threshold[0] = (threshold[0] + lo.l_value()) // 2
                threshold[1] = (threshold[1] + hi.l_value()) // 2
                threshold[2] = (threshold[2] + lo.a_value()) // 2
                threshold[3] = (threshold[3] + hi.a_value()) // 2
                threshold[4] = (threshold[4] + lo.b_value()) // 2
                threshold[5] = (threshold[5] + hi.b_value()) // 2
                for blob in img.find_blobs([threshold], pixels_threshold=100, area_threshold=100, merge=True, margin=10):
                    draw_img.draw_rectangle([v*SCALE for v in blob.rect()])
                    draw_img.draw_cross(blob.cx()*SCALE, blob.cy()*SCALE)
                    draw_img.draw_rectangle([v*SCALE for v in r])
                frame_count = frame_count + 1
                del hist
            else:
                for blob in img.find_blobs([threshold], pixels_threshold=100, area_threshold=100, merge=True, margin=10):
                    draw_img.draw_rectangle([v*SCALE for v in blob.rect()])
                    draw_img.draw_cross(blob.cx()*SCALE, blob.cy()*SCALE)

            # draw result to screen
            Display.show_image(draw_img)
            img = None
            gc.collect()
            if frame_count >= 120:
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
