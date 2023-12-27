from media.camera import *
from media.display import *
from media.media import *
from time import *
import time
import image
from mpp import *
import gc
from random import randint

def canmv_camera_test():
    print("canmv_camera_test")

    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)

    # use EVB LCD for display
    # display.init(HX8377_1080X1920_30FPS)

    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)

    out_width = 640
    out_height = 480
    out_width = ALIGN_UP(out_width, 16)

    # set chn0 output yuv420sp
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, out_width, out_height)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    meida_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    meida_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    media.create_link(meida_source, meida_sink)

    display.set_plane(0, 0, out_width, out_height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

    ret = media.buffer_init()
    if ret:
        print("canmv_camera_test, buffer init failed")
        return ret

    camera.start_stream(CAM_DEV_ID_0)
    img = None
    try:
        capture_count = 0
        while True:
            img = camera.capture_image(0, 0)
            img.compress_for_ide()
            camera.release_image(0, 0, img)
            gc.collect()
            capture_count += 1
            print(capture_count)
    except Exception as e:
        print(f"An error occurred during buffer used: {e}")
    finally:
        print('end')
        if img:
            camera.release_image(0, 0, img)
        else:
            print('img not dumped')
    camera.stop_stream(CAM_DEV_ID_0)

    display.deinit()

    media.destroy_link(meida_source, meida_sink)
    time.sleep(1)
    print("camera test exit")
    return 0

canmv_camera_test()
