import aicube                   #aicube模块，封装检测分割等任务相关后处理
from media.camera import *      #摄像头模块
from media.display import *     #显示模块
from media.media import *       #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区

import nncase_runtime as nn     #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
import ulab.numpy as np         #类似python numpy操作，但也会有一些接口不同

import time                     #时间统计
import image                    #图像模块，主要用于读取、图像绘制元素（框、点等）等操作

import gc                       #垃圾回收模块
import os, sys                  #操作系统接口模块

##config.py
#display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

##ai原图分辨率输入
OUT_RGB888P_WIDTH = ALIGN_UP(1920, 16)
OUT_RGB888P_HEIGHT = 1080

#kmodel输入shape
kmodel_input_shape = (1,3,512,512)                  # kmodel输入分辨率

#kmodel相关参数设置
confidence_threshold = 0.2                          # 手掌检测阈值，用于过滤roi
nms_threshold = 0.5                                 # 手掌检测框阈值，用于过滤重复roi
kmodel_frame_size = [512,512]                       # 手掌检测输入图片尺寸
frame_size = [OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGHT] # 直接输入图片尺寸
strides = [8,16,32]                                 # 输出特征图的尺寸与输入图片尺寸的比
num_classes = 1                                     # 模型输出类别数
nms_option = False                                  # 是否所有检测框一起做NMS，False则按照不同的类分别应用NMS
labels = ["hand"]                                   # 模型输出类别名称

root_dir = '/sdcard/app/tests/'
kmodel_file = root_dir + 'kmodel/hand_det.kmodel'   # kmodel文件的路径
anchors = [26,27, 53,52, 75,71, 80,99, 106,82, 99,134, 140,113, 161,172, 245,276]   #anchor设置

debug_mode = 0                                      # debug模式 大于0（调试）、 反之 （不调试）

#scoped_timing.py 用于debug模式输出程序块运行时间
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

#ai_utils.py
global current_kmodel_obj                                       # 定义全局的 kpu 对象
global ai2d,ai2d_input_tensor,ai2d_output_tensor,ai2d_builder   # 定义全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder


# ai2d 初始化
def ai2d_init():
    with ScopedTiming("ai2d_init",debug_mode > 0):
        global ai2d
        global ai2d_builder
        global ai2d_output_tensor
        # 计算padding值
        ori_w = OUT_RGB888P_WIDTH
        ori_h = OUT_RGB888P_HEIGHT
        width = kmodel_frame_size[0]
        height = kmodel_frame_size[1]
        ratiow = float(width) / ori_w
        ratioh = float(height) / ori_h
        if ratiow < ratioh:
            ratio = ratiow
        else:
            ratio = ratioh
        new_w = int(ratio * ori_w)
        new_h = int(ratio * ori_h)
        dw = float(width - new_w) / 2
        dh = float(height - new_h) / 2
        top = int(round(dh - 0.1))
        bottom = int(round(dh + 0.1))
        left = int(round(dw - 0.1))
        right = int(round(dw - 0.1))

        ai2d = nn.ai2d()
        ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        ai2d.set_pad_param(True, [0,0,0,0,top,bottom,left,right], 0, [114,114,114])
        ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
        ai2d_builder = ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1,3,height,width])
        data = np.ones(kmodel_input_shape, dtype=np.uint8)
        ai2d_output_tensor = nn.from_numpy(data)

# ai2d 运行
def ai2d_run(rgb888p_img):
    with ScopedTiming("ai2d_run",debug_mode > 0):
        global ai2d_input_tensor, ai2d_output_tensor, ai2d_builder
        ai2d_input = rgb888p_img.to_numpy_ref()
        ai2d_input_tensor = nn.from_numpy(ai2d_input)
        ai2d_builder.run(ai2d_input_tensor, ai2d_output_tensor)

# ai2d 释放内存
def ai2d_release():
    with ScopedTiming("ai2d_release",debug_mode > 0):
        global ai2d_input_tensor
        del ai2d_input_tensor

# kpu 初始化
def kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)

        ai2d_init()
        return kpu_obj

# kpu 输入预处理
def kpu_pre_process(rgb888p_img):
    ai2d_run(rgb888p_img)
    with ScopedTiming("kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,ai2d_output_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, ai2d_output_tensor)

# kpu 获得 kmodel 输出
def kpu_get_output():
    with ScopedTiming("kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()

            result = result.reshape((result.shape[0]*result.shape[1]*result.shape[2]*result.shape[3]))
            tmp2 = result.copy()
            del result
            results.append(tmp2)
        return results

# kpu 运行
def kpu_run(kpu_obj,rgb888p_img):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1)原图预处理，并设置模型输入
    kpu_pre_process(rgb888p_img)
    # (2)手掌检测 kpu 运行
    with ScopedTiming("kpu_run",debug_mode > 0):
        current_kmodel_obj.run()
    # (3)释放手掌检测 ai2d 资源
    ai2d_release()
    # (4)获取手掌检测 kpu 输出
    results = kpu_get_output()
    # (5)手掌检测 kpu 结果后处理
    dets = aicube.anchorbasedet_post_process( results[0], results[1], results[2], kmodel_frame_size, frame_size, strides, num_classes, confidence_threshold, nms_threshold, anchors, nms_option)
    # (6)返回手掌检测结果
    return dets

# kpu 释放内存
def kpu_deinit():
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        if 'ai2d' in globals():                             #删除ai2d变量，释放对它所引用对象的内存引用
            global ai2d
            del ai2d
        if 'ai2d_output_tensor' in globals():               #删除ai2d_output_tensor变量，释放对它所引用对象的内存引用
            global ai2d_output_tensor
            del ai2d_output_tensor
        if 'ai2d_builder' in globals():                     #删除ai2d_builder变量，释放对它所引用对象的内存引用
            global ai2d_builder
            del ai2d_builder

#media_utils.py
global draw_img,osd_img                                     #for display 定义全局 作图image对象
global buffer,media_source,media_sink                       #for media   定义 media 程序中的中间存储对象

#for display 初始化
def display_init():
    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

# display 释放内存
def display_deinit():
    display.deinit()

# display 作图过程 框出所有检测到的手以及标出得分
def display_draw(dets):
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img

        if dets:
            draw_img.clear()
            for det_box in dets:
                x1, y1, x2, y2 = det_box[2],det_box[3],det_box[4],det_box[5]
                w = float(x2 - x1) * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                h = float(y2 - y1) * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT

                x1 = int(x1 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH)
                y1 = int(y1 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT)
                x2 = int(x2 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH)
                y2 = int(y2 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT)

                if (h<(0.1*DISPLAY_HEIGHT)):
                    continue
                if (w<(0.25*DISPLAY_WIDTH) and ((x1<(0.03*DISPLAY_WIDTH)) or (x2>(0.97*DISPLAY_WIDTH)))):
                    continue
                if (w<(0.15*DISPLAY_WIDTH) and ((x1<(0.01*DISPLAY_WIDTH)) or (x2>(0.99*DISPLAY_WIDTH)))):
                    continue
                draw_img.draw_rectangle(x1 , y1 , int(w) , int(h), color=(255, 0, 255, 0), thickness = 2)
                draw_img.draw_string( x1 , y1-50, " " + labels[det_box[0]] + " " + str(round(det_box[1],2)), color=(255,0, 255, 0), scale=4)
            draw_img.copy_to(osd_img)
            display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)
        else:
            draw_img.clear()
            draw_img.copy_to(osd_img)
            display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)

#for camera 初始化
def camera_init(dev_id):
    camera.sensor_init(dev_id, CAM_DEFAULT_SENSOR)

    # set chn0 output yuv420sp
    camera.set_outsize(dev_id, CAM_CHN_ID_0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    camera.set_outfmt(dev_id, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    # set chn2 output rgb88planar
    camera.set_outsize(dev_id, CAM_CHN_ID_2, OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGHT)
    camera.set_outfmt(dev_id, CAM_CHN_ID_2, PIXEL_FORMAT_RGB_888_PLANAR)

# camera 开启
def camera_start(dev_id):
    camera.start_stream(dev_id)

# camera 读取图像
def camera_read(dev_id):
    with ScopedTiming("camera_read",debug_mode >0):
        rgb888p_img = camera.capture_image(dev_id, CAM_CHN_ID_2)
        return rgb888p_img

# camera 图像释放
def camera_release_image(dev_id,rgb888p_img):
    with ScopedTiming("camera_release_image",debug_mode >0):
        camera.release_image(dev_id, CAM_CHN_ID_2, rgb888p_img)

# camera 结束
def camera_stop(dev_id):
    camera.stop_stream(dev_id)

#for media 初始化
def media_init():
    config = k_vb_config()
    config.max_pool_cnt = 1
    config.comm_pool[0].blk_size = 4 * DISPLAY_WIDTH * DISPLAY_HEIGHT
    config.comm_pool[0].blk_cnt = 1
    config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE

    media.buffer_config(config)

    global media_source, media_sink
    media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    media_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    media.create_link(media_source, media_sink)

    # 初始化多媒体buffer
    media.buffer_init()

    global buffer, draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 图层1，用于画框
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    # 图层2，用于拷贝画框结果，防止画框过程中发生buffer搬运
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, poolid=buffer.pool_id, alloc=image.ALLOC_VB,
                          phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr)

# media 释放内存
def media_deinit():
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    if 'buffer' in globals():
        global buffer
        media.release_buffer(buffer)
    if 'media_source' in globals() and 'media_sink' in globals():
        global media_source, media_sink
        media.destroy_link(media_source, media_sink)

    media.buffer_deinit()

#**********for hand_detect.py**********
def hand_detect_inference():
    print("hand_detect_test start")
    kpu_hand_detect = kpu_init(kmodel_file)                             # 创建手掌检测的 kpu 对象
    camera_init(CAM_DEV_ID_0)                                           # 初始化 camera
    display_init()                                                      # 初始化 display

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)
        count = 0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                rgb888p_img = camera_read(CAM_DEV_ID_0)                 # 读取一帧图片

                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    dets = kpu_run(kpu_hand_detect,rgb888p_img)         # 执行手掌检测 kpu 运行 以及 后处理过程
                    display_draw(dets)                                  # 将得到的检测结果 绘制到 display

                camera_release_image(CAM_DEV_ID_0,rgb888p_img)          # camera 释放图像
                if (count>10):
                    gc.collect()
                    count = 0
                else:
                    count += 1

    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        camera_stop(CAM_DEV_ID_0)                                       # 停止 camera
        display_deinit()                                                # 释放 display
        kpu_deinit()                                                    # 释放 kpu
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_hand_detect
        gc.collect()
        nn.shrink_memory_pool()
        media_deinit()                                                  # 释放 整个media

    print("hand_detect_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    hand_detect_inference()

