import os
import ujson
from media import _sensor
from media.lcd import *
from media.media import *
from time import *
import nncase_runtime as nn
import ulab.numpy as np
import time
import utime
import image
import random
import gc
import sys

# 计时类，计算进入代码块和退出代码块的时间差
class ScopedTiming:
    def __init__(self, info="", enable_profile=True):
        self.info = info
        self.enable_profile = enable_profile

    def __enter__(self):
        if self.enable_profile:
            self.start_time = time.time_ns()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self.enable_profile:
            elapsed_time = time.time_ns() - self.start_time
            print(f"{self.info} took {elapsed_time / 1000000:.2f} ms")

# PipeLine类
class PipeLine:
    def __init__(self,rgb888p_size=[224,224],display_size=[1920,1080],display_mode="lcd"):
        # sensor给AI的图像分辨率
        self.rgb888p_size=[ALIGN_UP(rgb888p_size[0],16),rgb888p_size[1]]
        # 视频输出VO图像分辨率
        self.display_size=[ALIGN_UP(display_size[0],16),display_size[1]]
        # 视频显示模式，支持："lcd"，"hdmi"
        self.display_mode=display_mode
        # sensor对象
        self.sensor=None
        # osd显示Image对象
        self.osd_img=None
    
    # PipeLine初始化函数
    def create(self):
        os.exitpoint(os.EXITPOINT_ENABLE)
        nn.shrink_memory_pool()
        # 初始化显示
        if self.display_mode=="hdmi":
            lcd.init(LT9611_MIPI_4LAN_1920X1080_30FPS)
        else:
            lcd.init(ST7701_V1_MIPI_2LAN_480X800_30FPS)
        # 初始化并配置sensor
        self.sensor = _sensor.Sensor()
        self.sensor.reset()
        # 通道0直接给到显示VO，格式为YUV420
        self.sensor.set_framesize(self.display_size[0], self.display_size[1])
        self.sensor.set_pixformat(PIXEL_FORMAT_YUV_SEMIPLANAR_420)
        # 通道2给到AI做算法处理，格式为RGB888
        self.sensor.set_framesize(self.rgb888p_size[0], self.rgb888p_size[1], chn=CAM_CHN_ID_2)
        # set chn2 output format
        self.sensor.set_pixformat(PIXEL_FORMAT_RGB_888_PLANAR, chn=CAM_CHN_ID_2)
        # media初始化
        media.buffer_init(venc_cnt=0,wb_cnt=0)
        # OSD图像初始化
        self.osd_img = image.Image(self.display_size[0], self.display_size[1], image.ARGB8888)
        # 启动sensor
        self.sensor.run()
    
    # 获取一帧图像数据，返回格式为ulab的array数据
    def get_frame(self):
        frame = self.sensor.snapshot(chn=CAM_CHN_ID_2)
        input_np=frame.to_numpy_ref()
        return input_np
    
    # 在屏幕上显示osd_img
    def show_image(self):
        lcd.show_image(self.osd_img, 0, 0, DISPLAY_CHN_OSD3)
    
    # PipeLine销毁函数
    def destroy(self):
        os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
        # stop sensor
        self.sensor.stop()
        time.sleep_ms(50)
        # deinit lcd
        lcd.deinit()
        # deinit media buffer
        print("media deinit")
        media.buffer_deinit()