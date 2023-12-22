# Camera Example
#
# Note: You will need an SD card to run this example.
#
# You can start camera preview and capture yuv image.

from media.camera import *
from media.display import *
from media.media import *
import time, os

def camera_test():
    print("camera_test")
    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)
    out_width = 1920
    out_height = 1080
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
    out_width = 640
    out_height = 480
    out_width = ALIGN_UP(out_width, 16)
    # set chn1 output size
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_1, out_width, out_height)
    # set chn1 output format
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_1, PIXEL_FORMAT_RGB_888)
    # set chn1 output size
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_2, out_width, out_height)
    # set chn2 output format
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_2, PIXEL_FORMAT_RGB_888_PLANAR)
    # init meida buffer
    media.buffer_init()
    # start stream for camera device0
    camera.start_stream(CAM_DEV_ID_0)
    try:
        while True:
            os.exitpoint()
            time.sleep(5)
            for dev_num in range(CAM_DEV_ID_MAX):
                if not camera.cam_dev[dev_num].dev_attr.dev_enable:
                    continue

                for chn_num in range(CAM_CHN_ID_MAX):
                    if not camera.cam_dev[dev_num].chn_attr[chn_num].chn_enable:
                        continue

                    print(f"camera_test, dev({dev_num}) chn({chn_num}) capture frame.")
                    # capture image from dev and chn
                    img = camera.capture_image(dev_num, chn_num)
                    if img.format() == image.YUV420:
                        suffix = "yuv420sp"
                    elif img.format() == image.RGB888:
                        suffix = "rgb888"
                    elif img.format() == image.RGBP888:
                        suffix = "rgb888p"
                    else:
                        suffix = "unkown"

                    filename = f"/sdcard/dev_{dev_num:02d}_chn_{chn_num:02d}_{img.width()}x{img.height()}.{suffix}"
                    print("save capture image to file:", filename)
                    img.save(filename)
                    # release image for dev and chn
                    camera.release_image(dev_num, chn_num, img)
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
