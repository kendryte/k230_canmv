from media.display import *
from media.media import *
import time, os, sys, gc
import lvgl as lv

DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

def display_init():
    # use hdmi for display
    Display.init(Display.LT9611, to_ide = False)
    # init media manager
    MediaManager.init()

def display_deinit():
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(50)
    # deinit display
    Display.deinit()
    # release media buffer
    MediaManager.deinit()

def disp_drv_flush_cb(disp_drv, area, color):
    global buf1, buf2

    if disp_drv.flush_is_last() == True:
        if buf1.virtaddr() == uctypes.addressof(color.__dereference__()):
            Display.show_image(buf1)
        else:
            Display.show_image(buf2)
    disp_drv.flush_ready()

def lvgl_init():
    global buf1, buf2

    lv.init()
    disp_drv = lv.disp_create(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    disp_drv.set_flush_cb(disp_drv_flush_cb)
    buf1 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    buf2 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    disp_drv.set_draw_buffers(buf1.bytearray(), buf2.bytearray(), buf1.size(), lv.DISP_RENDER_MODE.DIRECT)

def lvgl_deinit():
    global buf1, buf2

    lv.deinit()
    del buf1
    del buf2

def user_gui_init():
    res_path = "/sdcard/app/tests/lvgl/data/"

    font_montserrat_16 = lv.font_load("A:" + res_path + "font/montserrat-16.fnt")
    ltr_label = lv.label(lv.scr_act())
    ltr_label.set_text("In modern terminology, a microcontroller is similar to a system on a chip (SoC).")
    ltr_label.set_style_text_font(font_montserrat_16,0)
    ltr_label.set_width(310)
    ltr_label.align(lv.ALIGN.TOP_MID, 0, 0)

    font_simsun_16_cjk = lv.font_load("A:" + res_path + "font/lv_font_simsun_16_cjk.fnt")
    cz_label = lv.label(lv.scr_act())
    cz_label.set_style_text_font(font_simsun_16_cjk, 0)
    cz_label.set_text("嵌入式系统（Embedded System），\n是一种嵌入机械或电气系统内部、具有专一功能和实时计算性能的计算机系统。")
    cz_label.set_width(310)
    cz_label.align(lv.ALIGN.BOTTOM_MID, 0, 0)

    anim_imgs = [None]*4
    with open(res_path + 'img/animimg001.png','rb') as f:
        anim001_data = f.read()

    anim_imgs[0] = lv.img_dsc_t({
    'data_size': len(anim001_data),
    'data': anim001_data
    })
    anim_imgs[-1] = anim_imgs[0]

    with open(res_path + 'img/animimg002.png','rb') as f:
        anim002_data = f.read()

    anim_imgs[1] = lv.img_dsc_t({
    'data_size': len(anim002_data),
    'data': anim002_data
    })

    with open(res_path + 'img/animimg003.png','rb') as f:
        anim003_data = f.read()

    anim_imgs[2] = lv.img_dsc_t({
    'data_size': len(anim003_data),
    'data': anim003_data
    })

    animimg0 = lv.animimg(lv.scr_act())
    animimg0.center()
    animimg0.set_src(anim_imgs, 4)
    animimg0.set_duration(2000)
    animimg0.set_repeat_count(lv.ANIM_REPEAT_INFINITE)
    animimg0.start()

def main():
    os.exitpoint(os.EXITPOINT_ENABLE)
    display_init()
    lvgl_init()
    try:
        user_gui_init()
        while True:
            time.sleep_ms(lv.task_handler())
    except BaseException as e:
        print(f"Exception {e}")
    lvgl_deinit()
    display_deinit()
    gc.collect()

if __name__ == "__main__":
    main()
