# Camera Example
#
# Note: You will need an SD card to run this example.
#
# You can start 3 camera preview.

from media.camera import *
from media.display import *
from media.media import *
import time, os
import sys

def camera_test():
    print("camera_test")
    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)
    out_width = 640
    out_height = 480
    # set camera out width align up with 16Bytes
    out_width = ALIGN_UP(out_width, 16)
    # set chn0 output size
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, out_width, out_height)
    # set chn0 out format
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # create meida source device
    meida_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    # create meida sink device
    meida_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    # create meida link
    media.create_link(meida_source, meida_sink)
    # set display plane with video channel
    display.set_plane(0, 0, out_width, out_height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)


    camera.sensor_init(CAM_DEV_ID_1, CAM_OV5647_1920X1080_CSI1_30FPS_10BIT_USEMCLK_LINEAR)
    out_width = 640
    out_height = 480
    # set camera out width align up with 16Bytes
    out_width = ALIGN_UP(out_width, 16)
    # set chn0 output size
    camera.set_outsize(CAM_DEV_ID_1, CAM_CHN_ID_0, out_width, out_height)
    # set chn0 out format
    camera.set_outfmt(CAM_DEV_ID_1, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # create meida source device
    meida_source1 = media_device(CAMERA_MOD_ID, CAM_DEV_ID_1, CAM_CHN_ID_0)
    # create meida sink device
    meida_sink1 = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO2)
    # create meida link
    media.create_link(meida_source1, meida_sink1)
    # set display plane with video channel
    display.set_plane(640, 320, out_width, out_height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO2)


    camera.sensor_init(CAM_DEV_ID_2, CAM_OV5647_1920X1080_CSI2_30FPS_10BIT_USEMCLK_LINEAR)
    out_width = 640
    out_height = 480
    # set camera out width align up with 16Bytes
    out_width = ALIGN_UP(out_width, 16)
    # set chn0 output size
    camera.set_outsize(CAM_DEV_ID_2, CAM_CHN_ID_0, out_width, out_height)
    # set chn0 out format
    camera.set_outfmt(CAM_DEV_ID_2, CAM_CHN_ID_0, PIXEL_FORMAT_RGB_888)
    # create meida source device
    meida_source2 = media_device(CAMERA_MOD_ID, CAM_DEV_ID_2, CAM_CHN_ID_0)
    # create meida sink device
    meida_sink2 = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_OSD0)
    # create meida link1
    media.create_link(meida_source2, meida_sink2)
    # set display plane with video channel
    display.set_plane(1280, 600, out_width, out_height, PIXEL_FORMAT_RGB_888, DISPLAY_MIRROR_NONE, DISPLAY_CHN_OSD0)


    # init meida buffer
    media.buffer_init()
    # start stream for camera device0
    camera.start_mcm_stream()

    try:
        while True:
            os.exitpoint()
            time.sleep(5)
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    # stop stream for camera device0
    camera.stop_mcm_stream()

    # deinit display
    display.deinit()
    # destroy media link
    media.destroy_link(meida_source, meida_sink)
    media.destroy_link(meida_source1, meida_sink1)
    media.destroy_link(meida_source2, meida_sink2)
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # deinit media buffer
    media.buffer_deinit()

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    camera_test()
