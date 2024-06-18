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

#--------for hand detection----------
#kmodel输入shape
hd_kmodel_input_shape = (1,3,512,512)                           # 手掌检测kmodel输入分辨率

#kmodel相关参数设置
confidence_threshold = 0.2                                      # 手掌检测阈值，用于过滤roi
nms_threshold = 0.5                                             # 手掌检测框阈值，用于过滤重复roi
hd_kmodel_frame_size = [512,512]                                # 手掌检测输入图片尺寸
hd_frame_size = [OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGHT]           # 手掌检测直接输入图片尺寸
strides = [8,16,32]                                             # 输出特征图的尺寸与输入图片尺寸的比
num_classes = 1                                                 # 手掌检测模型输出类别数
nms_option = False                                              # 是否所有检测框一起做NMS，False则按照不同的类分别应用NMS

root_dir = '/sdcard/app/tests/'
hd_kmodel_file = root_dir + "kmodel/hand_det.kmodel"      # 手掌检测kmodel文件的路径
anchors = [26,27, 53,52, 75,71, 80,99, 106,82, 99,134, 140,113, 161,172, 245,276]   #anchor设置

#--------for hand keypoint detection----------
#kmodel输入shape
hk_kmodel_input_shape = (1,3,256,256)                           # 手掌关键点检测kmodel输入分辨率

#kmodel相关参数设置
hk_kmodel_frame_size = [256,256]                                # 手掌关键点检测输入图片尺寸
hk_kmodel_file = root_dir + 'kmodel/handkp_det.kmodel'    # 手掌关键点检测kmodel文件的路径

debug_mode = 0                                                  # debug模式 大于0（调试）、 反之 （不调试）

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
global current_kmodel_obj                                                   # 定义全局的 kpu 对象
global hd_ai2d,hd_ai2d_input_tensor,hd_ai2d_output_tensor,hd_ai2d_builder   # 定义手掌检测全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder
global hk_ai2d,hk_ai2d_input_tensor,hk_ai2d_output_tensor,hk_ai2d_builder   # 定义手掌关键点检测全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder
global space_ai2d,space_ai2d_input_tensor,space_ai2d_output_tensor,space_ai2d_builder,space_draw_ai2d_release    # 定义缩放剪切图像全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder
space_draw_ai2d_release = False

#-------hand detect--------:
# 手掌检测ai2d 初始化
def hd_ai2d_init():
    with ScopedTiming("hd_ai2d_init",debug_mode > 0):
        global hd_ai2d
        global hd_ai2d_builder
        global hd_ai2d_output_tensor
        # 计算padding值
        ori_w = OUT_RGB888P_WIDTH
        ori_h = OUT_RGB888P_HEIGHT
        width = hd_kmodel_frame_size[0]
        height = hd_kmodel_frame_size[1]
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

        # init kpu and load kmodel
        hd_ai2d = nn.ai2d()
        hd_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        hd_ai2d.set_pad_param(True, [0,0,0,0,top,bottom,left,right], 0, [114,114,114])
        hd_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
        hd_ai2d_builder = hd_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1,3,height,width])
        data = np.ones(hd_kmodel_input_shape, dtype=np.uint8)
        hd_ai2d_output_tensor = nn.from_numpy(data)

# 手掌检测 ai2d 运行
def hd_ai2d_run(rgb888p_img):
    with ScopedTiming("hd_ai2d_run",debug_mode > 0):
        global hd_ai2d_input_tensor,hd_ai2d_output_tensor,hd_ai2d_builder
        hd_ai2d_input = rgb888p_img.to_numpy_ref()
        hd_ai2d_input_tensor = nn.from_numpy(hd_ai2d_input)

        hd_ai2d_builder.run(hd_ai2d_input_tensor, hd_ai2d_output_tensor)

# 手掌检测 ai2d 释放内存
def hd_ai2d_release():
    with ScopedTiming("hd_ai2d_release",debug_mode > 0):
        global hd_ai2d_input_tensor
        del hd_ai2d_input_tensor

# 手掌检测 kpu 初始化
def hd_kpu_init(hd_kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("hd_kpu_init",debug_mode > 0):
        hd_kpu_obj = nn.kpu()
        hd_kpu_obj.load_kmodel(hd_kmodel_file)

        hd_ai2d_init()
        return hd_kpu_obj

# 手掌检测 kpu 输入预处理
def hd_kpu_pre_process(rgb888p_img):
    hd_ai2d_run(rgb888p_img)
    with ScopedTiming("hd_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,hd_ai2d_output_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, hd_ai2d_output_tensor)

# 手掌检测 kpu 获得 kmodel 输出
def hd_kpu_get_output():
    with ScopedTiming("hd_kpu_get_output",debug_mode > 0):
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

# 手掌检测 kpu 运行
def hd_kpu_run(kpu_obj,rgb888p_img):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1)原图预处理，并设置模型输入
    hd_kpu_pre_process(rgb888p_img)
     # (2)手掌检测 kpu 运行
    with ScopedTiming("hd_kpu_run",debug_mode > 0):
        current_kmodel_obj.run()
    # (3)释放手掌检测 ai2d 资源
    hd_ai2d_release()
    # (4)获取手掌检测 kpu 输出
    results = hd_kpu_get_output()
    # (5)手掌检测 kpu 结果后处理
    dets = aicube.anchorbasedet_post_process( results[0], results[1], results[2], hd_kmodel_frame_size, hd_frame_size, strides, num_classes, confidence_threshold, nms_threshold, anchors, nms_option)
    # (6)返回手掌检测结果
    return dets

# 手掌检测 kpu 释放内存
def hd_kpu_deinit():
    with ScopedTiming("hd_kpu_deinit",debug_mode > 0):
        if 'hd_ai2d' in globals():
            global hd_ai2d
            del hd_ai2d
        if 'hd_ai2d_output_tensor' in globals():
            global hd_ai2d_output_tensor
            del hd_ai2d_output_tensor
        if 'hd_ai2d_builder' in globals():
            global hd_ai2d_builder
            del hd_ai2d_builder

#-------hand keypoint detection------:
# 手掌关键点检测 ai2d 初始化
def hk_ai2d_init():
    with ScopedTiming("hk_ai2d_init",debug_mode > 0):
        global hk_ai2d, hk_ai2d_output_tensor
        hk_ai2d = nn.ai2d()
        hk_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        data = np.ones(hk_kmodel_input_shape, dtype=np.uint8)
        hk_ai2d_output_tensor = nn.from_numpy(data)

# 手掌关键点检测 ai2d 运行
def hk_ai2d_run(rgb888p_img, x, y, w, h):
    with ScopedTiming("hk_ai2d_run",debug_mode > 0):
        global hk_ai2d,hk_ai2d_input_tensor,hk_ai2d_output_tensor
        hk_ai2d_input = rgb888p_img.to_numpy_ref()
        hk_ai2d_input_tensor = nn.from_numpy(hk_ai2d_input)

        hk_ai2d.set_crop_param(True, x, y, w, h)
        hk_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )

        global hk_ai2d_builder
        hk_ai2d_builder = hk_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1,3,hk_kmodel_frame_size[1],hk_kmodel_frame_size[0]])
        hk_ai2d_builder.run(hk_ai2d_input_tensor, hk_ai2d_output_tensor)

# 手掌关键点检测 ai2d 释放内存
def hk_ai2d_release():
    with ScopedTiming("hk_ai2d_release",debug_mode > 0):
        global hk_ai2d_input_tensor,hk_ai2d_builder
        del hk_ai2d_input_tensor
        del hk_ai2d_builder

# 手掌关键点检测 kpu 初始化
def hk_kpu_init(hk_kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("hk_kpu_init",debug_mode > 0):
        hk_kpu_obj = nn.kpu()
        hk_kpu_obj.load_kmodel(hk_kmodel_file)

        hk_ai2d_init()
        return hk_kpu_obj

# 手掌关键点检测 kpu 输入预处理
def hk_kpu_pre_process(rgb888p_img, x, y, w, h):
    hk_ai2d_run(rgb888p_img, x, y, w, h)
    with ScopedTiming("hk_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,hk_ai2d_output_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, hk_ai2d_output_tensor)

# 手掌关键点检测 kpu 获得 kmodel 输出
def hk_kpu_get_output():
    with ScopedTiming("hk_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()

            result = result.reshape((result.shape[0]*result.shape[1]))
            tmp2 = result.copy()
            del result
            results.append(tmp2)
        return results

# 手掌关键点检测 kpu 运行
def hk_kpu_run(kpu_obj,rgb888p_img, x, y, w, h):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1)原图预处理，并设置模型输入
    hk_kpu_pre_process(rgb888p_img, x, y, w, h)
    # (2)手掌关键点检测 kpu 运行
    with ScopedTiming("hk_kpu_run",debug_mode > 0):
        current_kmodel_obj.run()
    # (3)释放手掌关键点检测 ai2d 资源
    hk_ai2d_release()
    # (4)获取手掌关键点检测 kpu 输出
    results = hk_kpu_get_output()
    # (5)返回手掌关键点检测结果
    return results

# 手掌关键点检测 kpu 释放内存
def hk_kpu_deinit():
    with ScopedTiming("hk_kpu_deinit",debug_mode > 0):
        if 'hk_ai2d' in globals():
            global hk_ai2d
            del hk_ai2d
        if 'hk_ai2d_output_tensor' in globals():
            global hk_ai2d_output_tensor
            del hk_ai2d_output_tensor

# 隔空缩放剪切 ai2d 初始化
def space_ai2d_init():
    with ScopedTiming("space_ai2d_init",debug_mode > 0):
        global space_ai2d
        space_ai2d = nn.ai2d()
        space_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.RGB_packed,
                                       np.uint8, np.uint8)

# 隔空缩放剪切 ai2d 运行
def space_ai2d_run(rgb888p_img, x, y, w, h, out_w, out_h):
    with ScopedTiming("space_ai2d_run",debug_mode > 0):
        global space_ai2d,space_ai2d_input_tensor,space_ai2d_output_tensor,space_draw_ai2d_release
        space_draw_ai2d_release = True
        space_ai2d_input = rgb888p_img.to_numpy_ref()
        space_ai2d_input_tensor = nn.from_numpy(space_ai2d_input)

        space_ai2d.set_crop_param(True, x, y, w, h)
        space_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )

        data = np.ones((1,out_h, out_w,3), dtype=np.uint8)
        space_ai2d_output_tensor = nn.from_numpy(data)

        global space_ai2d_builder
        space_ai2d_builder = space_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1,out_h, out_w,3])
        space_ai2d_builder.run(space_ai2d_input_tensor, space_ai2d_output_tensor)

        space_np_out = space_ai2d_output_tensor.to_numpy()
        return space_np_out

# 隔空缩放剪切 ai2d 释放内存
def space_ai2d_release(re_ai2d):
    with ScopedTiming("space_ai2d_release",debug_mode > 0):
        global space_ai2d_input_tensor,space_ai2d_output_tensor,space_ai2d_builder,space_draw_ai2d_release,space_ai2d
        if (space_draw_ai2d_release):
            del space_ai2d_input_tensor
            del space_ai2d_output_tensor
            del space_ai2d_builder
            space_draw_ai2d_release = False
        if (re_ai2d):
            del space_ai2d

#media_utils.py
global draw_img,osd_img,masks                               #for display 定义全局 作图image对象
global buffer,media_source,media_sink                       #for media   定义 media 程序中的中间存储对象

#for display 初始化
def display_init():
    # use hdmi for display
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

# display 释放内存
def display_deinit():
    display.deinit()

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

    global buffer, draw_img, osd_img, masks
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 图层1，用于画框
    masks = np.zeros((DISPLAY_HEIGHT,DISPLAY_WIDTH,4),dtype=np.uint8)
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888,alloc=image.ALLOC_REF,data=masks)
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

#**********for space_resize.py**********
def space_resize_inference():
    print("space_resize start")
    kpu_hand_detect = hd_kpu_init(hd_kmodel_file)                       # 创建手掌检测的 kpu 对象
    kpu_hand_keypoint_detect = hk_kpu_init(hk_kmodel_file)              # 创建手掌关键点检测的 kpu 对象
    camera_init(CAM_DEV_ID_0)                                           # 初始化 camera
    display_init()                                                      # 初始化 display
    space_ai2d_init()                                                   # 初始化 隔空缩放剪切 ai2d 对象

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)

        global draw_img,osd_img
        first_start = True                                              # 首次手掌入镜参数
        two_point_left_x = 0                                            # 中指食指包括范围 x
        two_point_top_y = 0                                             # 中指食指包括范围 y
        two_point_mean_w = 0                                            # 中指食指首次入镜包括范围 w
        two_point_mean_h = 0                                            # 中指食指首次入镜包括范围 h
        two_point_crop_w = 0                                            # 中指食指包括范围 w
        two_point_crop_h = 0                                            # 中指食指包括范围 h
        osd_plot_x = 0                                                  # osd 画缩放图起始点 x
        osd_plot_y = 0                                                  # osd 画缩放图起始点 y
        ori_new_ratio = 0                                               # 缩放比例
        new_resize_w = 0                                                # 缩放后 w
        new_resize_h = 0                                                # 缩放后 h
        crop_area = 0                                                   # 剪切区域
        rect_frame_x = 0                                                # osd绘画起始点 x
        rect_frame_y = 0                                                # osd绘画起始点 y

        count = 0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                rgb888p_img = camera_read(CAM_DEV_ID_0)                 # 读取一帧图片

                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    two_point = np.zeros((4),dtype=np.int16)
                    dets_no_pro = hd_kpu_run(kpu_hand_detect,rgb888p_img)                      # 执行手掌检测 kpu 运行 以及 后处理过程
                    draw_img.clear()

                    dets = []
                    for det_box in dets_no_pro:
                        if det_box[4] < OUT_RGB888P_WIDTH - 10 :
                            dets.append(det_box)

                    if (len(dets)==1):
                        for det_box in dets:
                            x1, y1, x2, y2 = int(det_box[2]),int(det_box[3]),int(det_box[4]),int(det_box[5])
                            w = int(x2 - x1)
                            h = int(y2 - y1)

                            if (h<(0.1*OUT_RGB888P_HEIGHT)):
                                continue
                            if (w<(0.25*OUT_RGB888P_WIDTH) and ((x1<(0.03*OUT_RGB888P_WIDTH)) or (x2>(0.97*OUT_RGB888P_WIDTH)))):
                                continue
                            if (w<(0.15*OUT_RGB888P_WIDTH) and ((x1<(0.01*OUT_RGB888P_WIDTH)) or (x2>(0.99*OUT_RGB888P_WIDTH)))):
                                continue

                            length = max(w,h)/2
                            cx = (x1+x2)/2
                            cy = (y1+y2)/2
                            ratio_num = 1.26*length

                            x1_kp = int(max(0,cx-ratio_num))
                            y1_kp = int(max(0,cy-ratio_num))
                            x2_kp = int(min(OUT_RGB888P_WIDTH-1, cx+ratio_num))
                            y2_kp = int(min(OUT_RGB888P_HEIGHT-1, cy+ratio_num))
                            w_kp = int(x2_kp - x1_kp + 1)
                            h_kp = int(y2_kp - y1_kp + 1)

                            hk_results = hk_kpu_run(kpu_hand_keypoint_detect,rgb888p_img, x1_kp, y1_kp, w_kp, h_kp)     # 执行手掌关键点检测 kpu 运行 以及 后处理过程

                            results_show = np.zeros(hk_results[0].shape,dtype=np.int16)
                            results_show[0::2] = hk_results[0][0::2] * w_kp + x1_kp
                            results_show[1::2] = hk_results[0][1::2] * h_kp + y1_kp

                            two_point[0] = results_show[8]
                            two_point[1] = results_show[9]
                            two_point[2] = results_show[16+8]
                            two_point[3] = results_show[16+9]

                        if (first_start):
                            if (two_point[0] > 0 and two_point[0] < OUT_RGB888P_WIDTH and two_point[2] > 0 and two_point[2] < OUT_RGB888P_WIDTH and two_point[1] > 0 and two_point[1] < OUT_RGB888P_HEIGHT and two_point[3] > 0 and two_point[3] < OUT_RGB888P_HEIGHT):
                                two_point_mean_w = np.sqrt(pow(two_point[0] - two_point[2],2) + pow(two_point[1] - two_point[3],2))*0.8
                                two_point_mean_h = np.sqrt(pow(two_point[0] - two_point[2],2) + pow(two_point[1] - two_point[3],2))*0.8
                                first_start = False
                        else:
                            two_point_left_x = int(max((two_point[0] + two_point[2]) / 2 - two_point_mean_w / 2, 0))
                            two_point_top_y = int(max((two_point[1] + two_point[3]) / 2 - two_point_mean_h / 2, 0))
                            two_point_crop_w = int(min(min((two_point[0] + two_point[2]) / 2 - two_point_mean_w / 2 + two_point_mean_w , two_point_mean_w), OUT_RGB888P_WIDTH - ((two_point[0] + two_point[2]) / 2 - two_point_mean_w / 2)))
                            two_point_crop_h = int(min(min((two_point[1] + two_point[3]) / 2 - two_point_mean_h / 2 + two_point_mean_h , two_point_mean_h), OUT_RGB888P_HEIGHT - ((two_point[1] + two_point[3]) / 2 - two_point_mean_h / 2)))

                            ori_new_ratio = np.sqrt(pow((two_point[0] - two_point[2]),2) + pow((two_point[1] - two_point[3]),2))*0.8 / two_point_mean_w

                            new_resize_w = min(int(two_point_crop_w * ori_new_ratio / OUT_RGB888P_WIDTH * DISPLAY_WIDTH),600)
                            new_resize_h = min(int(two_point_crop_h * ori_new_ratio / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT),600)

                            rect_frame_x = int(two_point_left_x * 1.0 / OUT_RGB888P_WIDTH * DISPLAY_WIDTH)
                            rect_frame_y = int(two_point_top_y * 1.0 / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT)

                            draw_w = min(new_resize_w,DISPLAY_WIDTH-rect_frame_x-1)
                            draw_h = min(new_resize_h,DISPLAY_HEIGHT-rect_frame_y-1)

                            space_np_out = space_ai2d_run(rgb888p_img, two_point_left_x, two_point_top_y, two_point_crop_w, two_point_crop_h, new_resize_w, new_resize_h)      # 运行 隔空缩放检测 ai2d
                            global masks
                            masks[rect_frame_y:rect_frame_y + draw_h,rect_frame_x:rect_frame_x + draw_w,0] = 255
                            masks[rect_frame_y:rect_frame_y + draw_h,rect_frame_x:rect_frame_x + draw_w,1:4] = space_np_out[0][0:draw_h,0:draw_w,:]
                            space_ai2d_release(False)                       # 释放 隔空缩放检测 ai2d 相关对象


                            draw_img.draw_rectangle(rect_frame_x, rect_frame_y, new_resize_w, new_resize_h, color=(255, 0, 255, 0),thickness = 4)
                    else:
                        draw_img.draw_string( 300 , 500, "Must have one hand !", color=(255,255,0,0), scale=7)
                        first_start = True

                camera_release_image(CAM_DEV_ID_0,rgb888p_img)         # camera 释放图像

                if (count > 5):
                    gc.collect()
                    count = 0
                else:
                    count += 1

                draw_img.copy_to(osd_img)
                display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:

        camera_stop(CAM_DEV_ID_0)                                       # 停止 camera
        display_deinit()                                                # 释放 display
        space_ai2d_release(True)                                        # 释放 隔空缩放检测 ai2d 相关对象
        hd_kpu_deinit()                                  # 释放手掌检测 kpu
        hk_kpu_deinit()                         # 释放手掌关键点检测 kpu
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_hand_detect
        del kpu_hand_keypoint_detect
        
        if 'draw_img' in globals():
            global draw_img
            del draw_img
        if 'masks' in globals():
            global masks
            del masks
        gc.collect()
        nn.shrink_memory_pool()
        media_deinit()                                            # 释放 整个media

    print("space_resize end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    space_resize_inference()
