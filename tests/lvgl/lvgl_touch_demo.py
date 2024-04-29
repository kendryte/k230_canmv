from media.display import *
from media.media import *
import time, os, sys, gc
import lvgl as lv
from machine import TOUCH

DISPLAY_WIDTH = ALIGN_UP(480, 16)
DISPLAY_HEIGHT = 800

def display_init():
    # use hdmi for display
    display.init(2)
    # config vb for osd layer
    config = k_vb_config()
    config.max_pool_cnt = 1
    config.comm_pool[0].blk_size = 4*DISPLAY_WIDTH*DISPLAY_HEIGHT
    config.comm_pool[0].blk_cnt = 1
    config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
    # meida buffer config
    media.buffer_config(config)
    # media buffer init
    media.buffer_init()
    # request media buffer for osd image
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # create image for osd
    globals()["buffer"] = buffer
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, alloc=image.ALLOC_VB, phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr, poolid=buffer.pool_id)
    globals()["osd_img"] = osd_img
    osd_img.clear()
    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD0)

def display_deinit():
    # deinit display
    display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # release media buffer
    media.release_buffer(globals()["buffer"])
    # deinit media buffer
    media.buffer_deinit()

def disp_drv_flush_cb(disp_drv, area, color):
    if disp_drv.flush_is_last() == True:
        osd_img = globals()["osd_img"]
        osd_img.copy_from(color.__dereference__(osd_img.size()))
    disp_drv.flush_ready()

class touch_screen():
    def __init__(self):
        self.x = 0
        self.y = 0
        self.state = lv.INDEV_STATE.RELEASED

        self.indev_drv = lv.indev_create()
        self.indev_drv.set_type(lv.INDEV_TYPE.POINTER)
        self.indev_drv.set_read_cb(self.callback)
        self.touch = TOUCH(0)

    def callback(self, driver, data):
        x, y, state = self.x, self.y, lv.INDEV_STATE.RELEASED
        tp = self.touch.read(1)
        if len(tp):
            x, y, event = tp[0].x, tp[0].y, tp[0].event
            if event == 2 or event == 3:
                state = lv.INDEV_STATE.PRESSED
        data.point = lv.point_t({'x': x, 'y': y})
        data.state = state

def lvgl_init():
    lv.init()
    disp_drv = lv.disp_create(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    disp_drv.set_flush_cb(disp_drv_flush_cb)
    buf1 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    buf2 = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    disp_drv.set_draw_buffers(buf1.bytearray(), buf2.bytearray(), buf1.size(), lv.DISP_RENDER_MODE.DIRECT)
    globals()["buf1"] = buf1
    globals()["buf2"] = buf2
    tp = touch_screen()

def lvgl_deinit():
    lv.deinit()
    del globals()["buf1"]
    del globals()["buf2"]

def btn_clicked_event(event):
    btn = lv.btn.__cast__(event.get_target())
    label = lv.label.__cast__(btn.get_user_data())
    if "on" == label.get_text():
        label.set_text("off")
    else:
        label.set_text("on")

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

    btn = lv.btn(lv.scr_act())
    btn.align(lv.ALIGN.CENTER, 0, lv.pct(25))
    label = lv.label(btn)
    label.set_text('on')
    btn.set_user_data(label)
    btn.add_event(btn_clicked_event, lv.EVENT.CLICKED, None)

def main():
    os.exitpoint(os.EXITPOINT_ENABLE)
    display_init()
    lvgl_init()
    try:
        user_gui_init()
        while True:
            time.sleep_ms(lv.task_handler())
    except BaseException as e:
        sys.print_exception(e)
    lvgl_deinit()
    display_deinit()
    gc.collect()

if __name__ == "__main__":
    main()
