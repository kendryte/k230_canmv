# Find Data Matrices Example
#
# This example shows off how easy it is to detect data matrices using the
# CanMV Cam M7. Data matrices detection does not work on the M4 Camera.

from media.camera import *
from media.display import *
from media.media import *
import time, math, os, gc

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
    # set chn1 output nv12
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_1, 480, 270)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_1, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
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
    time.sleep_ms(100)
    media.buffer_deinit()

def capture_picture():
    fps = time.clock()
    while True:
        fps.tick()
        try:
            os.exit_exception_mask(1)
            yuv420_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
            if yuv420_img == -1:
                # release image for dev and chn
                camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, yuv420_img)
                os.exit_exception_mask(0)
                raise OSError("camera capture image failed")
            else:
                img = image.Image(yuv420_img.width(), yuv420_img.height(), image.GRAYSCALE, alloc=image.ALLOC_HEAP, data=yuv420_img)
                camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, yuv420_img)
                os.exit_exception_mask(0)
                matrices = img.find_datamatrices()
                for matrix in matrices:
                    print_args = (matrix.rows(), matrix.columns(), matrix.payload(), (180 * matrix.rotation()) / math.pi, fps.fps())
                    print("Matrix [%d:%d], Payload \"%s\", rotation %f (degrees), FPS %f" % print_args)
                if not matrices:
                    print("FPS %f" % fps.fps())
                del img
                gc.collect()
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