# Camera Example
#
# Note: You will need an SD card to run this example.
#
# You can start camera preview and capture yuv image.

from media.camera import *
from media.display import *
from media.media import *
import time, os
import sys

def camera_test():
    print("camera_test")
    # use hdmi for display
    display.init(ST7701_V1_MIPI_2LAN_480X800_30FPS)
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)

    # set mirror
    # camera.set_hmirror()
    # set flip
    # camera.set_vflip()

    out_width = 800
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
    display.set_plane(0, 0, out_height, out_width, PIXEL_FORMAT_YVU_PLANAR_420, K_ROTATION_90, DISPLAY_CHN_VIDEO1)

    # init meida buffer
    media.buffer_init()
    # start stream for camera device0
    camera.start_stream(CAM_DEV_ID_0)
    try:
        while True:
            os.exitpoint()
            time.sleep(5)
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    # stop stream for camera device0
    camera.stop_stream(CAM_DEV_ID_0)
    # deinit display
    display.deinit()
    # destroy media link
    media.destroy_link(meida_source, meida_sink)
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # deinit media buffer
    media.buffer_deinit()

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    camera_test()
