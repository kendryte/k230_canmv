from media.camera import *
from media.display import *
from media.media import *
from time import *
import time


def camera_display_test():
    CAM_OUTPUT_BUF_NUM = 6
    CAM_INPUT_BUF_NUM = 4

    out_width = 1080
    out_height = 720
    out_width = ALIGN_UP(out_width, 16)

    display.init(LT9611_1920X1080_30FPS)
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)

    # set chn0
    camera.set_outbufs(CAM_DEV_ID_0, CAM_CHN_ID_0, CAM_OUTPUT_BUF_NUM)
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, out_width, out_height)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    meida_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    meida_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    media.create_link(meida_source, meida_sink)

    display.set_plane(400, 200, out_width, out_height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

    ret = media.buffer_init()
    if ret:
        print("camera_display_test, buffer init failed")
        return ret

    camera.start_stream(CAM_DEV_ID_0)
    count = 0
    while count < 600:
        time.sleep(1)
        count += 1

    camera.stop_stream(CAM_DEV_ID_0)
    media.destroy_link(meida_source, meida_sink)
    time.sleep(1)
    display.deinit()
    ret = media.buffer_deinit()
    if ret:
        print("camera_display_test, media_buffer_deinit failed")
    return ret

    print("camera_display_test exit")
    return 0

camera_display_test()
