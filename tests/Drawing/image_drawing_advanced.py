# Draw Image Testing script with bounce
#
# Exercise draw image with many different values for testing

from media.camera import *
from media.display import *
from media.media import *
import time, os, gc, sys

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

BOUNCE = True
RESCALE = True

SMALL_IMAGE_SCALE = 3

CYCLE_FORMATS = True
CYCLE_MASK = True

# Used when CYCLE_FORMATS or CYCLE_MASK is true
value_mixer_init = 0

# Location of small image
x_init = 100
y_init = 50

# Bounce direction
xd_init = 1
yd_init = 1

# Small image scaling
rescale_init = 1.0
rd_init = 0.1
max_rescale = 5
min_rescale = 0.2

# Boundary to bounce within
xmin = -DISPLAY_WIDTH / SMALL_IMAGE_SCALE - 8
ymin = -DISPLAY_HEIGHT / SMALL_IMAGE_SCALE - 8
xmax = DISPLAY_WIDTH + 8
ymax = DISPLAY_HEIGHT + 8

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
    # # set chn0 output size
    # camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    # # set chn0 output format
    # camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # # create meida source device
    # globals()["meida_source"] = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    # # create meida sink device
    # globals()["meida_sink"] = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    # # create meida link
    # media.create_link(meida_source, meida_sink)
    # # set display plane with video channel
    # display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)
    # set chn1 output nv12
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_1, DISPLAY_WIDTH, DISPLAY_HEIGHT)
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
    # media.destroy_link(globals()["meida_source"], globals()["meida_sink"])
    # deinit media buffer
    media.buffer_deinit()

def draw():
    # create image for osd
    buffer = globals()["buffer"]
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.RGB565, alloc=image.ALLOC_VB, phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr, poolid=buffer.pool_id)
    osd_img.clear()
    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD0)
    value_mixer = value_mixer_init
    x = x_init
    y = y_init
    xd = xd_init
    yd = yd_init
    rescale = rescale_init
    rd = rd_init
    fps = time.clock()
    while True:
        fps.tick()
        status = ""
        value_mixer = value_mixer + 1
        try:
            os.exitpoint()
            rgb888_img = camera.capture_image(CAM_DEV_ID_0, CAM_CHN_ID_1)
            img = rgb888_img.to_rgb565()
            camera.release_image(CAM_DEV_ID_0, CAM_CHN_ID_1, rgb888_img)
            small_img = img.mean_pooled(SMALL_IMAGE_SCALE, SMALL_IMAGE_SCALE)
            status = 'rgb565 '
            if CYCLE_FORMATS:
                image_format = (value_mixer >> 8) & 3
                # To test combining different formats
                if (image_format==1): small_img = small_img.to_bitmap(); status = 'bitmap '
                if (image_format==2): small_img = small_img.to_grayscale(); status = 'grayscale '
                if (image_format==3): small_img = small_img.to_rgb565(); status = 'rgb565 '

            # update small image location
            if BOUNCE:
                x = x + xd
                if (x<xmin or x>xmax):
                    xd = -xd

                y = y + yd
                if (y<ymin or y>ymax):
                    yd = -yd

            # Update small image scale
            if RESCALE:
                rescale = rescale + rd
                if (rescale<min_rescale or rescale>max_rescale):
                    rd = -rd

            # Find the center of the image
            scaled_width = int(small_img.width() * abs(rescale))
            scaled_height= int(small_img.height() * abs(rescale))

            apply_mask = CYCLE_MASK and ((value_mixer >> 9) & 1)
            if apply_mask:
                img.draw_image(small_img, int(x), int(y), mask=small_img.to_bitmap(), x_scale=rescale, y_scale=rescale, alpha=240)
                status += 'alpha:240 '
                status += '+mask '
            else:
                img.draw_image(small_img, int(x), int(y), x_scale=rescale, y_scale=rescale, alpha=128)
                status += 'alpha:128 '

            img.draw_string(8, 0, status, mono_space = False)
            img.copy_to(osd_img)
            del img, small_img
            gc.collect()
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
        print("draw")
        draw()
    except Exception as e:
        sys.print_exception(e)
    finally:
        if camera_is_init:
            print("camera deinit")
            camera_deinit()

if __name__ == "__main__":
    main()
