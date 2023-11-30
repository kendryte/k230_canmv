# Emboss Snapshot Example
#
# Note: You will need an SD card to run this example.
#
# You can use your CanMV Cam to save modified image files.

from media.camera import *
from media.display import *
from media.media import *
from time import *
import os

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

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

    # set chn1 output rgb888
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_1, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_1, PIXEL_FORMAT_RGB_888)

    # media buffer init
    media.buffer_init()
    # start stream for camera device0
    camera.start_stream(CAM_DEV_ID_0)

def camera_deinit():
    # stop stream for camera device0
    camera.stop_stream(CAM_DEV_ID_0)
    # deinit display
    display.deinit()
    # destroy media link
    media.destroy_link(globals()["meida_source"], globals()["meida_sink"])
    # deinit media buffer
    media.buffer_deinit()

def capture_picture():
    sleep(1)
    rgb888_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
    if rgb888_img == -1:
        print("capture_image failed")
    else:
        try:
            img = rgb888_img.to_rgb565()
            img.morph(1, [+2, +1, +0,\
                          +1, +1, -1,\
                          +0, -1, -2]) # Emboss the image.
            img.save("/sdcard/snapshot_emboss.jpg")
            print("save image ok")
        except Exception as e:
            print("save image fail: ", e)
    # release image for dev and chn
    camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, rgb888_img)

def main():
    os.exit_exception_mask(1)
    print("camera init")
    camera_init()
    print("camera capture")
    capture_picture()
    print("camera deinit")
    camera_deinit()
    os.exit_exception_mask(0)

if __name__ == "__main__":
    main()
