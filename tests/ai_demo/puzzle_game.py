import aicube                   #aicube模块，封装检测分割等任务相关后处理
from media.camera import *      #摄像头模块
from media.display import *     #显示模块
from media.media import *       #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区

import nncase_runtime as nn     #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
import ulab.numpy as np         #类似python numpy操作，但也会有一些接口不同

import time                     #时间统计
import image                    #图像模块，主要用于读取、图像绘制元素（框、点等）等操作

import gc                       #垃圾回收模块
import random
import os, sys                           #操作系统接口模块

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

level = 3                                                       # 游戏级别 目前只支持设置为 3


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

#**********for puzzle_game.py**********
def puzzle_game_inference():
    print("puzzle_game_inference start")
    kpu_hand_detect = hd_kpu_init(hd_kmodel_file)                       # 创建手掌检测的 kpu 对象
    kpu_hand_keypoint_detect = hk_kpu_init(hk_kmodel_file)              # 创建手掌关键点检测的 kpu 对象
    camera_init(CAM_DEV_ID_0)                                           # 初始化 camera
    display_init()                                                      # 初始化 display

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)

        global draw_img,osd_img
        puzzle_width = DISPLAY_HEIGHT                                   # 设定 拼图宽
        puzzle_height = DISPLAY_HEIGHT                                  # 设定 拼图高
        puzzle_ori_width = DISPLAY_WIDTH - puzzle_width - 50            # 设定 原始拼图宽
        puzzle_ori_height = DISPLAY_WIDTH - puzzle_height - 50          # 设定 原始拼图高

        every_block_width = int(puzzle_width/level)                     # 设定 拼图块宽
        every_block_height = int(puzzle_height/level)                   # 设定 拼图块高
        ori_every_block_width = int(puzzle_ori_width/level)             # 设定 原始拼图宽
        ori_every_block_height = int(puzzle_ori_height/level)           # 设定 原始拼图高
        ratio_num = every_block_width/360.0                             # 字体比例
        blank_x = 0                                                     # 空白块 角点x
        blank_y = 0                                                     # 空白块 角点y
        direction_vec = [-1,1,-1,1]                                     # 空白块四种移动方向

        exact_division_x = 0                                            # 交换块 角点x
        exact_division_y = 0                                            # 交换块 角点y
        distance_tow_points = DISPLAY_WIDTH                             # 两手指距离
        distance_thred = every_block_width*0.4                          # 两手指距离阈值

        move_mat = np.zeros((every_block_height,every_block_width,4),dtype=np.uint8)

        osd_frame_tmp = np.zeros((DISPLAY_HEIGHT,DISPLAY_WIDTH,4),dtype=np.uint8)
        osd_frame_tmp_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888,alloc=image.ALLOC_REF,data=osd_frame_tmp)
        osd_frame_tmp[0:puzzle_height,0:puzzle_width,3] = 100
        osd_frame_tmp[0:puzzle_height,0:puzzle_width,2] = 150
        osd_frame_tmp[0:puzzle_height,0:puzzle_width,1] = 130
        osd_frame_tmp[0:puzzle_height,0:puzzle_width,0] = 127
        osd_frame_tmp[(1080-puzzle_ori_height)//2:(1080-puzzle_ori_height)//2+puzzle_ori_width,puzzle_width+25:puzzle_width+25+puzzle_ori_height,3] = 100
        osd_frame_tmp[(1080-puzzle_ori_height)//2:(1080-puzzle_ori_height)//2+puzzle_ori_width,puzzle_width+25:puzzle_width+25+puzzle_ori_height,2] = 150
        osd_frame_tmp[(1080-puzzle_ori_height)//2:(1080-puzzle_ori_height)//2+puzzle_ori_width,puzzle_width+25:puzzle_width+25+puzzle_ori_height,1] = 130
        osd_frame_tmp[(1080-puzzle_ori_height)//2:(1080-puzzle_ori_height)//2+puzzle_ori_width,puzzle_width+25:puzzle_width+25+puzzle_ori_height,0] = 127
        for i in range(level*level):
            osd_frame_tmp_img.draw_rectangle((i%level)*every_block_width,(i//level)*every_block_height,every_block_width,every_block_height,(255,0,0,0),5)
            osd_frame_tmp_img.draw_string((i%level)*every_block_width + 55,(i//level)*every_block_height + 45,str(i),(255,0,0,255),30*ratio_num)
            osd_frame_tmp_img.draw_rectangle(puzzle_width+25 + (i%level)*ori_every_block_width,(1080-puzzle_ori_height)//2 + (i//level)*ori_every_block_height,ori_every_block_width,ori_every_block_height,(255,0,0,0),5)
            osd_frame_tmp_img.draw_string(puzzle_width+25 + (i%level)*ori_every_block_width + 50,(1080-puzzle_ori_height)//2 + (i//level)*ori_every_block_height + 25,str(i),(255,0,0,255),20*ratio_num)
        osd_frame_tmp[0:every_block_height,0:every_block_width,3] = 114
        osd_frame_tmp[0:every_block_height,0:every_block_width,2] = 114
        osd_frame_tmp[0:every_block_height,0:every_block_width,1] = 114
        osd_frame_tmp[0:every_block_height,0:every_block_width,0] = 220

        for i in range(level*10):
            k230_random = int(random.random() * 100) % 4
            blank_x_tmp = blank_x
            blank_y_tmp = blank_y
            if (k230_random < 2):
                blank_x_tmp = blank_x + direction_vec[k230_random]
            else:
                blank_y_tmp = blank_y + direction_vec[k230_random]

            if ((blank_x_tmp >= 0 and blank_x_tmp < level) and (blank_y_tmp >= 0 and blank_y_tmp < level) and (abs(blank_x - blank_x_tmp) <= 1 and abs(blank_y - blank_y_tmp) <= 1)):
                move_rect = [blank_x_tmp*every_block_width,blank_y_tmp*every_block_height,every_block_width,every_block_height]
                blank_rect = [blank_x*every_block_width,blank_y*every_block_height,every_block_width,every_block_height]

                move_mat[:] = osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:]
                osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:] = osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:]
                osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:] = move_mat[:]

                blank_x = blank_x_tmp
                blank_y = blank_y_tmp

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

                    osd_frame_tmp_img.copy_to(draw_img)

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
                            results_show[0::2] = (hk_results[0][0::2] * w_kp + x1_kp) #* DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                            results_show[1::2] = (hk_results[0][1::2] * h_kp + y1_kp) #* DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT

                            two_point[0] = results_show[8+8]
                            two_point[1] = results_show[8+9]
                            two_point[2] = results_show[16+8]
                            two_point[3] = results_show[16+9]

                        if (two_point[1] <= OUT_RGB888P_WIDTH):
                            distance_tow_points = np.sqrt(pow((two_point[0]-two_point[2]),2) + pow((two_point[1] - two_point[3]),2))* 1.0 / OUT_RGB888P_WIDTH * DISPLAY_WIDTH
                            exact_division_x = int((two_point[0] * 1.0 / OUT_RGB888P_WIDTH * DISPLAY_WIDTH)//every_block_width)
                            exact_division_y = int((two_point[1] * 1.0 / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT)//every_block_height)


                            if (distance_tow_points < distance_thred and exact_division_x >= 0 and exact_division_x < level and exact_division_y >= 0 and exact_division_y < level):
                                if (abs(blank_x - exact_division_x) == 1 and abs(blank_y - exact_division_y) == 0):
                                    move_rect = [exact_division_x*every_block_width,exact_division_y*every_block_height,every_block_width,every_block_height]
                                    blank_rect = [blank_x*every_block_width,blank_y*every_block_height,every_block_width,every_block_height]

                                    move_mat[:] = osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:]
                                    osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:] = osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:]
                                    osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:] = move_mat[:]

                                    blank_x = exact_division_x
                                elif (abs(blank_y - exact_division_y) == 1 and abs(blank_x - exact_division_x) == 0):
                                    move_rect = [exact_division_x*every_block_width,exact_division_y*every_block_height,every_block_width,every_block_height]
                                    blank_rect = [blank_x*every_block_width,blank_y*every_block_height,every_block_width,every_block_height]

                                    move_mat[:] = osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:]
                                    osd_frame_tmp[move_rect[1]:move_rect[1]+move_rect[3],move_rect[0]:move_rect[0]+move_rect[2],:] = osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:]
                                    osd_frame_tmp[blank_rect[1]:blank_rect[1]+blank_rect[3],blank_rect[0]:blank_rect[0]+blank_rect[2],:] = move_mat[:]

                                    blank_y = exact_division_y

                                osd_frame_tmp_img.copy_to(draw_img)
                                x1 = int(two_point[0] * 1.0 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH)
                                y1 = int(two_point[1] * 1.0 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT)
                                draw_img.draw_circle(x1, y1, 1, color=(255, 0, 255, 255),thickness=4,fill=False)
                            else:
                                osd_frame_tmp_img.copy_to(draw_img)
                                x1 = int(two_point[0] * 1.0 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH)
                                y1 = int(two_point[1] * 1.0 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT)
                                draw_img.draw_circle(x1, y1, 1, color=(255, 255, 255, 0),thickness=4,fill=False)
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
        hd_kpu_deinit()                                                 # 释放手掌检测 kpu
        hk_kpu_deinit()                                                 # 释放手掌关键点检测 kpu
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

    print("puzzle_game_inference end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    puzzle_game_inference()

