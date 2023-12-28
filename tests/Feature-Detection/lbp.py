# Local Binary Patterns (LBP) Example
#
# This example shows off how to use the local binary pattern feature descriptor
# on your CanMV Cam. LBP descriptors work like Freak feature descriptors.
#
# WARNING: LBP supports needs to be reworked! As of right now this feature needs
# a lot of work to be made into somethin useful. This script will reamin to show
# that the functionality exists, but, in its current state is inadequate.

from media.camera import *
from media.display import *
from media.media import *
import time, os, gc, sys

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
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_1, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
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

def camera_drop(frame):
    for i in range(frame):
        os.exitpoint()
        img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
        camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, img)

def capture_picture():
    # create image for drawing
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    # create image for osd
    buffer = globals()["buffer"]
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, alloc=image.ALLOC_VB, phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr, poolid=buffer.pool_id)
    osd_img.clear()
    osd_img.draw_string(0, 0, "Please wait...")
    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD0)
    # Load Haar Cascade
    # By default this will use all stages, lower satges is faster but less accurate.
    face_cascade = image.HaarCascade("frontalface", stages=25)
    print(face_cascade)
    d0 = None
    #d0 = image.load_descriptor("/desc.lbp")
    # Skip a few frames to allow the sensor settle down
    # Note: This takes more time when exec from the IDE.
    camera_drop(90)
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exitpoint()
            yuv420_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
            img = image.Image(yuv420_img.width(), yuv420_img.height(), image.GRAYSCALE, data=yuv420_img)
            camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, yuv420_img)
            draw_img.clear()
            objects = img.find_features(face_cascade, threshold=0.5, scale_factor=1.25)
            if objects:
                face = objects[0]
                d1 = img.find_lbp(face)
                if (d0 == None):
                    d0 = d1
                else:
                    dist = image.match_descriptor(d0, d1)
                    draw_img.draw_string(0, 10, "Match %d%%"%(dist))
                    print("Match %d%%"%(dist))

                draw_img.draw_rectangle([v*SCALE for v in face])
            # Draw FPS
            draw_img.draw_string(0, 0, "FPS:%.2f"%(fps.fps()))
            draw_img.copy_to(osd_img)
            del img
            gc.collect()
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
