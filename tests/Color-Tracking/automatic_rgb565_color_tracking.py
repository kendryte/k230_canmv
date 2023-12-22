# Automatic RGB565 Color Tracking Example
#
# This example shows off single color automatic RGB565 color tracking using the CanMV Cam.

from media.camera import *
from media.display import *
from media.media import *
import time, os, gc, sys, math

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

def camera_init():
    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)
    # config vb for osd layer
    config = k_vb_config()
    config.max_pool_cnt = 1
    config.comm_pool[0].blk_size = 4*DISPLAY_WIDTH*DISPLAY_HEIGHT
    config.comm_pool[0].blk_cnt = 1
    config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
    # meida buffer config
    media.buffer_config(config)
    # init default sensor
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)
    # set chn0 output size
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    # set chn0 output format
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # create meida source device
    globals()["meida_source"] = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    # create meida sink device
    globals()["meida_sink"] = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    # create meida link
    media.create_link(meida_source, meida_sink)
    # set display plane with video channel
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)
    # set chn1 output nv12
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_1, DETECT_WIDTH, DETECT_HEIGHT)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_1, PIXEL_FORMAT_RGB_888)
    # media buffer init
    media.buffer_init()
    # request media buffer for osd image
    globals()["buffer"] = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # start stream for camera device0
    camera.start_stream(CAM_DEV_ID_0)

def camera_deinit():
    # stop stream for camera device0
    camera.stop_stream(CAM_DEV_ID_0)
    # deinit display
    display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # release media buffer
    media.release_buffer(globals()["buffer"])
    # destroy media link
    media.destroy_link(globals()["meida_source"], globals()["meida_sink"])
    # deinit media buffer
    media.buffer_deinit()

def capture_picture():
    # create image for drawing
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    # create image for osd
    buffer = globals()["buffer"]
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, alloc=image.ALLOC_VB, phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr, poolid=buffer.pool_id)
    osd_img.clear()
    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD0)
    # Capture the color thresholds for whatever was in the center of the image.
    r = [(DETECT_WIDTH//2)-(50//2), (DETECT_HEIGHT//2)-(50//2), 50, 50] # 50x50 center of QVGA.
    threshold = [50, 50, 0, 0, 0, 0] # Middle L, A, B values.
    frame_count = 0
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exitpoint()
            rgb888_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
            img = rgb888_img.to_rgb565()
            camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, rgb888_img)
            draw_img.clear()
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

            draw_img.copy_to(osd_img)
            del img
            gc.collect()
            if frame_count >= 120:
                print(fps.fps())
        except KeyboardInterrupt as e:
            print("user stop: ", e)
            break
        except BaseException as e:
            sys.print_exception(e)
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
        sys.print_exception(e)
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
