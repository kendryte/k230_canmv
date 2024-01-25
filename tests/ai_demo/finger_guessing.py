import ulab.numpy as np                 #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn             #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *              #摄像头模块
from media.display import *             #显示模块
from media.media import *               #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
from random import randint              #随机整数生成
import image                            #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                             #时间统计
import gc                               #垃圾回收模块
import aicube                           #aicube模块，封装ai cube 相关后处理
import os, sys                          #操作系统接口模块

##config.py
#display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

#ai原图分辨率输入
OUT_RGB888P_WIDTH = ALIGN_UP(1920, 16)
OUT_RGB888P_HEIGHT = 1080

#手掌检测 和 手掌关键点检测 kmodel输入shape
hd_kmodel_input_shape = (1,3,512,512)
hk_kmodel_input_shape = (1,3,256,256)

#手掌检测 相关参数设置
confidence_threshold = 0.2                                  #手掌检测 分数阈值
nms_threshold = 0.5                                         #非极大值抑制 阈值
hd_kmodel_frame_size = [512,512]                            #手掌检测kmodel输入  w h
hd_frame_size = [OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGHT]       #手掌检测原始输入图像 w h
strides = [8,16,32]                                         #手掌检测模型 下采样输出倍数
num_classes = 1                                             #检测类别数， 及手掌一种
nms_option = False                                          #控制最大值抑制的方式 False 类内  True 类间
labels = ["hand"]                                           #标签名称
anchors = [26,27,53,52,75,71,80,99,106,82,99,134,140,113,161,172,245,276]   #手掌检测模型 锚框
#手掌关键点检测 相关参数
hk_kmodel_frame_size = [256,256]                            #手掌关键点检测 kmodel 输入 w h

# kmodel 路径
root_dir = '/sdcard/app/tests/'
hd_kmodel_file = root_dir + 'kmodel/hand_det.kmodel'          #手掌检测kmodel路径
hk_kmodel_file = root_dir + 'kmodel/handkp_det.kmodel'        #手掌关键点kmodel路径
debug_mode = 0                                                          # debug模式 大于0（调试）、 反之 （不调试）

# 猜拳模式  0 玩家稳赢 ， 1 玩家必输 ， n > 2 多局多胜
guess_mode = 3

# 读取石头剪刀布的bin文件方法
def read_file(file_name):
    image_arr = np.fromfile(file_name,dtype=np.uint8)
    image_arr = image_arr.reshape((400,400,4))
    return image_arr
# 石头剪刀布的 array
five_image = read_file(root_dir + "utils/five.bin")
fist_image = read_file(root_dir + "utils/fist.bin")
shear_image = read_file(root_dir + "utils/shear.bin")


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
global current_kmodel_obj                                                       # 定义全局的 kpu 对象
global hd_ai2d,hd_ai2d_input_tensor,hd_ai2d_output_tensor,hd_ai2d_builder       # 定义手掌检测 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder
global hk_ai2d,hk_ai2d_input_tensor,hk_ai2d_output_tensor,hk_ai2d_builder       # 定义手掌关键点检测 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder
global counts_guess, player_win, k230_win, sleep_end, set_stop_id               # 定义猜拳游戏的参数：猜拳次数、玩家赢次、k230赢次、是否停顿、是狗暂停

# 手掌检测 ai2d 初始化
def hd_ai2d_init():
    with ScopedTiming("hd_ai2d_init",debug_mode > 0):
        global hd_ai2d
        global hd_ai2d_builder
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

        global hd_ai2d_output_tensor
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

# 手掌检测 kpu 获取 kmodel 输出
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
    # (1) 原始图像预处理，并设置模型输入
    hd_kpu_pre_process(rgb888p_img)
    # (2) kpu 运行
    with ScopedTiming("hd_kpu_run",debug_mode > 0):
        current_kmodel_obj.run()
    # (3) 释放ai2d资源
    hd_ai2d_release()
    # (4) 获取kpu输出
    results = hd_kpu_get_output()
    # (5) kpu结果后处理
    dets = aicube.anchorbasedet_post_process( results[0], results[1], results[2], hd_kmodel_frame_size, hd_frame_size, strides, num_classes, confidence_threshold, nms_threshold, anchors, nms_option)
    # (6) 返回 手掌检测 结果
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

# 手掌关键点检测 ai2d 初始化
def hk_ai2d_init():
    with ScopedTiming("hk_ai2d_init",debug_mode > 0):
        global hk_ai2d
        hk_ai2d = nn.ai2d()
        hk_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)

        global hk_ai2d_output_tensor
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

# 手掌关键点检测 接收kmodel结果的后处理
def hk_kpu_post_process(results, x, y, w, h):
    results_show = np.zeros(results.shape,dtype=np.int16)
    # results_show = np.zeros(len(results),dtype=np.int16)
    results_show[0::2] = results[0::2] * w + x
    results_show[1::2] = results[1::2] * h + y
    return results_show

# 手掌关键点检测 kpu 运行
def hk_kpu_run(kpu_obj,rgb888p_img, x, y, w, h):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    hk_kpu_pre_process(rgb888p_img, x, y, w, h)
    # (2) kpu 运行
    with ScopedTiming("hk_kpu_run",debug_mode > 0):
        current_kmodel_obj.run()
    # (3) 释放ai2d资源
    hk_ai2d_release()
    # (4) 获取kpu输出
    results = hk_kpu_get_output()
    # (5) kpu结果后处理
    result = hk_kpu_post_process(results[0],x,y,w,h)
    # (6) 返回 关键点检测 结果
    return result

# 手掌关键点检测 kpu 释放内存
def hk_kpu_deinit():
    with ScopedTiming("hk_kpu_deinit",debug_mode > 0):
        if 'hk_ai2d' in globals():
            global hk_ai2d
            del hk_ai2d
        if 'hk_ai2d_output_tensor' in globals():
            global hk_ai2d_output_tensor
            del hk_ai2d_output_tensor

# 手掌关键点检测 计算角度
def hk_vector_2d_angle(v1,v2):
    v1_x = v1[0]
    v1_y = v1[1]
    v2_x = v2[0]
    v2_y = v2[1]
    v1_norm = np.sqrt(v1_x * v1_x+ v1_y * v1_y)
    v2_norm = np.sqrt(v2_x * v2_x + v2_y * v2_y)
    dot_product = v1_x * v2_x + v1_y * v2_y
    cos_angle = dot_product/(v1_norm*v2_norm)
    angle = np.acos(cos_angle)*180/np.pi
    # if (angle>180):
    #     return 65536
    return angle

# 利用手掌关键点检测的结果 判断手掌手势
def hk_gesture(kpu_hand_keypoint_detect,rgb888p_img,det_box):
    x1, y1, x2, y2 = int(det_box[2]),int(det_box[3]),int(det_box[4]),int(det_box[5])
    w = int(x2 - x1)
    h = int(y2 - y1)

    if (h<(0.1*OUT_RGB888P_HEIGHT)):
        return
    if (w<(0.25*OUT_RGB888P_WIDTH) and ((x1<(0.03*OUT_RGB888P_WIDTH)) or (x2>(0.97*OUT_RGB888P_WIDTH)))):
        return
    if (w<(0.15*OUT_RGB888P_WIDTH) and ((x1<(0.01*OUT_RGB888P_WIDTH)) or (x2>(0.99*OUT_RGB888P_WIDTH)))):
        return

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

    results = hk_kpu_run(kpu_hand_keypoint_detect,rgb888p_img, x1_kp, y1_kp, w_kp, h_kp)

    angle_list = []
    for i in range(5):
        angle = hk_vector_2d_angle([(results[0]-results[i*8+4]), (results[1]-results[i*8+5])],[(results[i*8+6]-results[i*8+8]),(results[i*8+7]-results[i*8+9])])
        angle_list.append(angle)

    thr_angle = 65.
    thr_angle_thumb = 53.
    thr_angle_s = 49.
    gesture_str = None
    if 65535. not in angle_list:
        if (angle_list[0]>thr_angle_thumb)  and (angle_list[1]>thr_angle) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]>thr_angle):
            gesture_str = "fist"
        elif (angle_list[0]<thr_angle_s)  and (angle_list[1]<thr_angle_s) and (angle_list[2]<thr_angle_s) and (angle_list[3]<thr_angle_s) and (angle_list[4]<thr_angle_s):
            gesture_str = "five"
        elif (angle_list[0]<thr_angle_s)  and (angle_list[1]<thr_angle_s) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]>thr_angle):
            gesture_str = "gun"
        elif (angle_list[0]<thr_angle_s)  and (angle_list[1]<thr_angle_s) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]<thr_angle_s):
            gesture_str = "love"
        elif (angle_list[0]>5)  and (angle_list[1]<thr_angle_s) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]>thr_angle):
            gesture_str = "one"
        elif (angle_list[0]<thr_angle_s)  and (angle_list[1]>thr_angle) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]<thr_angle_s):
            gesture_str = "six"
        elif (angle_list[0]>thr_angle_thumb)  and (angle_list[1]<thr_angle_s) and (angle_list[2]<thr_angle_s) and (angle_list[3]<thr_angle_s) and (angle_list[4]>thr_angle):
            gesture_str = "three"
        elif (angle_list[0]<thr_angle_s)  and (angle_list[1]>thr_angle) and (angle_list[2]>thr_angle) and (angle_list[3]>thr_angle) and (angle_list[4]>thr_angle):
            gesture_str = "thumbUp"
        elif (angle_list[0]>thr_angle_thumb)  and (angle_list[1]<thr_angle_s) and (angle_list[2]<thr_angle_s) and (angle_list[3]>thr_angle) and (angle_list[4]>thr_angle):
            gesture_str = "yeah"

    return gesture_str


#media_utils.py
global draw_img,osd_img,masks                                               #for display 定义全局 作图image对象
global buffer,media_source,media_sink                                       #for media  定义 media 程序中的中间存储对象

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


#**********for finger_guessing.py**********
def finger_guessing_inference():
    print("finger_guessing_test start")
    kpu_hand_detect = hd_kpu_init(hd_kmodel_file)                                       # 创建手掌检测的 kpu 对象
    kpu_hand_keypoint_detect = hk_kpu_init(hk_kmodel_file)                              # 创建手掌关键点检测的 kpu 对象
    camera_init(CAM_DEV_ID_0)                                                           # 初始化 camera
    display_init()                                                                      # 初始化 display

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)                                                      # 开启 camera
        counts_guess = -1                                                               # 猜拳次数 计数
        player_win = 0                                                                  # 玩家 赢次计数
        k230_win = 0                                                                    # k230 赢次计数
        sleep_end = False                                                               # 是否 停顿
        set_stop_id = True                                                              # 是否 暂停猜拳
        LIBRARY = ["fist","yeah","five"]                                                # 猜拳 石头剪刀布 三种方案的dict

        count = 0
        global draw_img,masks,osd_img
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()

            with ScopedTiming("total",1):
                rgb888p_img = camera_read(CAM_DEV_ID_0)                                 # 读取一帧图像

                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    with ScopedTiming("trigger time", debug_mode > 0):
                        dets_no_pro = hd_kpu_run(kpu_hand_detect,rgb888p_img)                  # 执行手掌检测 kpu 运行 以及 后处理过程
                        gesture = ""
                        draw_img.clear()

                        dets = []
                        for det_box in dets_no_pro:
                            if det_box[4] < OUT_RGB888P_WIDTH - 10 :
                                dets.append(det_box)

                        for det_box in dets:
                            gesture = hk_gesture(kpu_hand_keypoint_detect,rgb888p_img,det_box)      # 执行手掌关键点检测 kpu 运行 以及 后处理过程 得到手势类型
                        if (len(dets) >= 2):
                            draw_img.draw_string( 300 , 500, "Must have one hand !", color=(255,255,0,0), scale=7)
                            draw_img.copy_to(osd_img)
                        elif (guess_mode == 0):
                            if (gesture == "fist"):
                                masks[:400,:400,:] = shear_image
                            elif (gesture == "five"):
                                masks[:400,:400,:] = fist_image
                            elif (gesture == "yeah"):
                                masks[:400,:400,:] = five_image
                            draw_img.copy_to(osd_img)
                        elif (guess_mode == 1):
                            if (gesture == "fist"):
                                masks[:400,:400,:] = five_image
                            elif (gesture == "five"):
                                masks[:400,:400,:] = shear_image
                            elif (gesture == "yeah"):
                                masks[:400,:400,:] = fist_image
                            draw_img.copy_to(osd_img)
                        else:
                            if (sleep_end):
                                time.sleep_ms(2000)
                                sleep_end = False
                            if (len(dets) == 0):
                                set_stop_id = True
                            if (counts_guess == -1 and gesture != "fist" and gesture != "yeah" and gesture != "five"):
                                draw_img.draw_string( 400 , 450, "G A M E   S T A R T", color=(255,255,0,0), scale=7)
                                draw_img.draw_string( 400 , 550, "     1   S E T     ", color=(255,255,0,0), scale=7)
                                draw_img.copy_to(osd_img)
                            elif (counts_guess == guess_mode):
                                draw_img.clear()
                                if (k230_win > player_win):
                                    draw_img.draw_string( 400 , 450, "Y O U    L O S E", color=(255,255,0,0), scale=7)
                                elif (k230_win < player_win):
                                    draw_img.draw_string( 400 , 450, "Y O U    W I N", color=(255,255,0,0), scale=7)
                                else:
                                    draw_img.draw_string( 400 , 450, "T I E    G A M E", color=(255,255,0,0), scale=7)
                                draw_img.copy_to(osd_img)
                                counts_guess = -1
                                player_win = 0
                                k230_win = 0

                                sleep_end = True
                            else:
                                if (set_stop_id):
                                    if (counts_guess == -1 and (gesture == "fist" or gesture == "yeah" or gesture == "five")):
                                        counts_guess = 0
                                    if (counts_guess != -1 and (gesture == "fist" or gesture == "yeah" or gesture == "five")):
                                        k230_guess = randint(1,10000) % 3
                                        if (gesture == "fist" and LIBRARY[k230_guess] == "yeah"):
                                            player_win += 1
                                        elif (gesture == "fist" and LIBRARY[k230_guess] == "five"):
                                            k230_win += 1
                                        if (gesture == "yeah" and LIBRARY[k230_guess] == "fist"):
                                            k230_win += 1
                                        elif (gesture == "yeah" and LIBRARY[k230_guess] == "five"):
                                            player_win += 1
                                        if (gesture == "five" and LIBRARY[k230_guess] == "fist"):
                                            player_win += 1
                                        elif (gesture == "five" and LIBRARY[k230_guess] == "yeah"):
                                            k230_win += 1

                                        if (LIBRARY[k230_guess] == "fist"):
                                            masks[:400,:400,:] = fist_image
                                        elif (LIBRARY[k230_guess] == "five"):
                                            masks[:400,:400,:] = five_image
                                        elif (LIBRARY[k230_guess] == "yeah"):
                                            masks[:400,:400,:] = shear_image

                                        counts_guess += 1;
                                        draw_img.draw_string( 400 , 450, "     " + str(counts_guess) + "   S E T     ", color=(255,255,0,0), scale=7)
                                        draw_img.copy_to(osd_img)
                                        set_stop_id = False
                                        sleep_end = True

                                    else:
                                        draw_img.draw_string( 400 , 450, "     " + str(counts_guess+1) + "   S E T     ", color=(255,255,0,0), scale=7)
                                        draw_img.copy_to(osd_img)
                                else:
                                    draw_img.copy_to(osd_img)
                        display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)                             # 将得到的图像 绘制到 display

                camera_release_image(CAM_DEV_ID_0,rgb888p_img)                                          # camera 释放图形

                if (count > 5):
                    gc.collect()
                    count = 0
                else:
                    count += 1

    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        camera_stop(CAM_DEV_ID_0)                           # 停止 camera
        display_deinit()                                    # 停止 display
        hd_kpu_deinit()                                     # 释放手掌检测 kpu
        hk_kpu_deinit()                                     # 释放手掌关键点检测 kpu

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
        media_deinit()                                # 释放 整个 media


    print("finger_guessing_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    finger_guessing_inference()

