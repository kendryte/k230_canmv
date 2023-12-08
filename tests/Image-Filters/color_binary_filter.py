# Color Binary Filter Example
#
# This script shows off the binary image filter. You may pass binary any
# number of thresholds to segment the image by.

from media.camera import *
from media.display import *
from media.media import *
import time, os, gc

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080
SCALE = 4
DETECT_WIDTH = DISPLAY_WIDTH // SCALE
DETECT_HEIGHT = DISPLAY_HEIGHT // SCALE

# Use the Tools -> Machine Vision -> Threshold Edtor to pick better thresholds.
red_threshold = (0,100,   0,127,   0,127) # L A B
green_threshold = (0,100,   -128,0,   0,127) # L A B
blue_threshold = (0,100,   -128,127,   -128,0) # L A B

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
    ret = media.buffer_config(config)
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
    time.sleep_ms(100)
    # release media buffer
    media.release_buffer(globals()["buffer"])
    # destroy media link
    media.destroy_link(globals()["meida_source"], globals()["meida_sink"])
    # deinit media buffer
    media.buffer_deinit()

def capture_picture():
    # create image for osd
    buffer = globals()["buffer"]
    osd_img = image.Image(DETECT_WIDTH, DETECT_HEIGHT, image.RGB565, alloc=image.ALLOC_VB, phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr, poolid=buffer.pool_id)
    osd_img.clear()
    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD0)
    frame_count = 0
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exit_exception_mask(1)
            rgb888_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
            if rgb888_img == -1:
                # release image for dev and chn
                camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, rgb888_img)
                os.exit_exception_mask(0)
                raise OSError("camera capture image failed")
            else:
                img = rgb888_img.to_rgb565()
                camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, rgb888_img)
                os.exit_exception_mask(0)
                # Test red threshold
                if frame_count < 100:
                    img.binary([red_threshold])
                # Test green threshold
                elif frame_count < 200:
                    img.binary([green_threshold])
                # Test blue threshold
                elif frame_count < 300:
                    img.binary([blue_threshold])
                # Test not red threshold
                elif frame_count < 400:
                    img.binary([red_threshold], invert = 1)
                # Test not green threshold
                elif frame_count < 500:
                    img.binary([green_threshold], invert = 1)
                # Test not blue threshold
                elif frame_count < 600:
                    img.binary([blue_threshold], invert = 1)
                else:
                    frame_count = 0
                frame_count = frame_count + 1
                img.copy_to(osd_img)
                del img
                gc.collect()
                print(fps.fps())
        except Exception as e:
            print(e)
            break

def main():
    camera_is_init = False
    try:
        os.exit_exception_mask(1)
        print("camera init")
        camera_init()
        camera_is_init = True
        os.exit_exception_mask(0)
        print("camera capture")
        capture_picture()
    except Exception as e:
        os.exit_exception_mask(1)
        print(e)
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()
        os.exit_exception_mask(0)

if __name__ == "__main__":
    main()