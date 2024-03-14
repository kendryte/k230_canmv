import ulab.numpy as np             #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn         #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *          #摄像头模块
from media.display import *         #显示模块
from media.media import *           #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import image                        #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                         #时间统计
import gc                           #垃圾回收模块
import aidemo                       #aidemo模块，封装ai demo相关后处理、画图操作
import os, sys                      #操作系统接口模块

##config.py
#display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

#ai原图分辨率输入
OUT_RGB888P_WIDTH = ALIGN_UP(1280, 16)
OUT_RGB888P_HEIGHT = 720

#单目标跟踪 kmodel 输入 shape
crop_kmodel_input_shape = (1,3,127,127)
src_kmodel_input_shape = (1,3,255,255)


#单目标跟踪 相关参数设置
head_thresh = 0.1                                           #单目标跟踪分数阈值
CONTEXT_AMOUNT = 0.5                                        #跟踪框宽、高调整系数
rgb_mean = [114,114,114]                                    #padding颜色值
ratio_src_crop = float(src_kmodel_input_shape[2])/float(crop_kmodel_input_shape[2])     #src模型和crop模型输入比值
track_x1 = float(300)                                       #起始跟踪目标框左上角点x
track_y1 = float(300)                                       #起始跟踪目标框左上角点y
track_w = float(100)                                        #起始跟踪目标框w
track_h = float(100)                                        #起始跟踪目标框h


#文件配置
root_dir = '/sdcard/app/tests/'
crop_kmodel_file = root_dir + 'kmodel/cropped_test127.kmodel'                               #单目标跟踪 crop kmodel 文件路径
src_kmodel_file = root_dir + 'kmodel/nanotrack_backbone_sim.kmodel'                         #单目标跟踪 src kmodel 文件路径
track_kmodel_file = root_dir + 'kmodel/nanotracker_head_calib_k230.kmodel'                  #单目标跟踪 head kmodel 文件路径

debug_mode = 0                                                                              # debug模式 大于0（调试）、 反之 （不调试）

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
global current_kmodel_obj                                                                               # 定义全局的 kpu 对象
global crop_ai2d,crop_ai2d_input_tensor,crop_ai2d_output_tensor,crop_ai2d_builder                       # 对应crop模型： ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder
global crop_pad_ai2d,crop_pad_ai2d_input_tensor,crop_pad_ai2d_output_tensor,crop_pad_ai2d_builder       # 对应crop模型： ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder
global src_ai2d,src_ai2d_input_tensor,src_ai2d_output_tensor,src_ai2d_builder                           # 对应src模型： ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder
global src_pad_ai2d,src_pad_ai2d_input_tensor,src_pad_ai2d_output_tensor,src_pad_ai2d_builder           # 对应src模型： ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder
global track_kpu_input_0,track_kpu_input_1                                                              # 对应head模型： 两个输入


# 单目标跟踪的后处理
def track_kpu_post_process(output_data,center_xy_wh):
    with ScopedTiming("track_kpu_post_process", debug_mode > 0):
        det = aidemo.nanotracker_postprocess(output_data[0],output_data[1],[OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH],head_thresh,center_xy_wh,crop_kmodel_input_shape[2],CONTEXT_AMOUNT)
        return det

# 单目标跟踪 对应crop模型的 ai2d 初始化
def crop_ai2d_init():
    with ScopedTiming("crop_ai2d_init",debug_mode > 0):
        global crop_ai2d, crop_pad_ai2d
        crop_ai2d = nn.ai2d()
        crop_pad_ai2d = nn.ai2d()

        crop_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)
        crop_pad_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)

        crop_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
        crop_pad_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        global crop_ai2d_out_tensor
        data = np.ones(crop_kmodel_input_shape, dtype=np.uint8)
        crop_ai2d_out_tensor = nn.from_numpy(data)

# 单目标跟踪 对应crop模型的 ai2d 运行
def crop_ai2d_run(rgb888p_img,center_xy_wh):
    with ScopedTiming("crop_ai2d_run",debug_mode > 0):
        global crop_ai2d, crop_pad_ai2d
        global crop_ai2d_input_tensor,crop_ai2d_out_tensor,crop_ai2d_builder
        global crop_pad_ai2d_input_tensor,crop_pad_ai2d_out_tensor,crop_pad_ai2d_builder

        s_z = round(np.sqrt((center_xy_wh[2] + CONTEXT_AMOUNT * (center_xy_wh[2] + center_xy_wh[3])) * (center_xy_wh[3] + CONTEXT_AMOUNT * (center_xy_wh[2] + center_xy_wh[3]))))
        c = (s_z + 1) / 2
        context_xmin = np.floor(center_xy_wh[0] - c + 0.5)
        context_xmax = int(context_xmin + s_z - 1)
        context_ymin = np.floor(center_xy_wh[1] - c + 0.5)
        context_ymax = int(context_ymin + s_z - 1)

        left_pad = int(max(0, -context_xmin))
        top_pad = int(max(0, -context_ymin))
        right_pad = int(max(0, int(context_xmax - OUT_RGB888P_WIDTH + 1)))
        bottom_pad = int(max(0, int(context_ymax - OUT_RGB888P_HEIGHT + 1)))
        context_xmin = context_xmin + left_pad
        context_xmax = context_xmax + left_pad
        context_ymin = context_ymin + top_pad
        context_ymax = context_ymax + top_pad

        if (left_pad != 0 or right_pad != 0 or top_pad != 0 or bottom_pad != 0):
            crop_pad_ai2d.set_pad_param(True, [0, 0, 0, 0, top_pad, bottom_pad, left_pad, right_pad], 0, rgb_mean)
            crop_pad_ai2d_builder = crop_pad_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad])
            crop_pad_ai2d_input = rgb888p_img.to_numpy_ref()
            crop_pad_ai2d_input_tensor = nn.from_numpy(crop_pad_ai2d_input)
            crop_pad_ai2d_output = np.ones([1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad], dtype=np.uint8)
            crop_pad_ai2d_out_tensor = nn.from_numpy(crop_pad_ai2d_output)
            crop_pad_ai2d_builder.run(crop_pad_ai2d_input_tensor, crop_pad_ai2d_out_tensor)

            crop_ai2d.set_crop_param(True, int(context_xmin), int(context_ymin), int(context_xmax - context_xmin + 1), int(context_ymax - context_ymin + 1))
            crop_ai2d_builder = crop_ai2d.build([1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad], crop_kmodel_input_shape)
            crop_ai2d_input_tensor = crop_pad_ai2d_out_tensor
            crop_ai2d_builder.run(crop_ai2d_input_tensor, crop_ai2d_out_tensor)
            del crop_pad_ai2d_input_tensor
            del crop_pad_ai2d_out_tensor
            del crop_pad_ai2d_builder
        else:
            crop_ai2d.set_crop_param(True, int(center_xy_wh[0] - s_z/2.0), int(center_xy_wh[1] - s_z/2.0), int(s_z), int(s_z))
            crop_ai2d_builder = crop_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], crop_kmodel_input_shape)
            crop_ai2d_input = rgb888p_img.to_numpy_ref()
            crop_ai2d_input_tensor = nn.from_numpy(crop_ai2d_input)
            crop_ai2d_builder.run(crop_ai2d_input_tensor, crop_ai2d_out_tensor)

# 单目标跟踪 对应crop模型的 ai2d 释放
def crop_ai2d_release():
    with ScopedTiming("crop_ai2d_release",debug_mode > 0):
        global crop_ai2d_input_tensor,crop_ai2d_builder
        del crop_ai2d_input_tensor
        del crop_ai2d_builder


# 单目标跟踪 对应src模型的 ai2d 初始化
def src_ai2d_init():
    with ScopedTiming("src_ai2d_init",debug_mode > 0):
        global src_ai2d, src_pad_ai2d
        src_ai2d = nn.ai2d()
        src_pad_ai2d = nn.ai2d()

        src_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)
        src_pad_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)

        src_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel )
        src_pad_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        global src_ai2d_out_tensor
        data = np.ones(src_kmodel_input_shape, dtype=np.uint8)
        src_ai2d_out_tensor = nn.from_numpy(data)

# 单目标跟踪 对应src模型的 ai2d 运行
def src_ai2d_run(rgb888p_img,center_xy_wh):
    with ScopedTiming("src_ai2d_run",debug_mode > 0):
        global src_ai2d, src_pad_ai2d
        global src_ai2d_input_tensor,src_ai2d_out_tensor,src_ai2d_builder
        global src_pad_ai2d_input_tensor,src_pad_ai2d_out_tensor,src_pad_ai2d_builder

        s_z = round(np.sqrt((center_xy_wh[2] + CONTEXT_AMOUNT * (center_xy_wh[2] + center_xy_wh[3])) * (center_xy_wh[3] + CONTEXT_AMOUNT * (center_xy_wh[2] + center_xy_wh[3])))) * ratio_src_crop
        c = (s_z + 1) / 2
        context_xmin = np.floor(center_xy_wh[0] - c + 0.5)
        context_xmax = int(context_xmin + s_z - 1)
        context_ymin = np.floor(center_xy_wh[1] - c + 0.5)
        context_ymax = int(context_ymin + s_z - 1)

        left_pad = int(max(0, -context_xmin))
        top_pad = int(max(0, -context_ymin))
        right_pad = int(max(0, int(context_xmax - OUT_RGB888P_WIDTH + 1)))
        bottom_pad = int(max(0, int(context_ymax - OUT_RGB888P_HEIGHT + 1)))
        context_xmin = context_xmin + left_pad
        context_xmax = context_xmax + left_pad
        context_ymin = context_ymin + top_pad
        context_ymax = context_ymax + top_pad

        if (left_pad != 0 or right_pad != 0 or top_pad != 0 or bottom_pad != 0):
            src_pad_ai2d.set_pad_param(True, [0, 0, 0, 0, top_pad, bottom_pad, left_pad, right_pad], 0, rgb_mean)
            src_pad_ai2d_builder = src_pad_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], [1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad])
            src_pad_ai2d_input = rgb888p_img.to_numpy_ref()
            src_pad_ai2d_input_tensor = nn.from_numpy(src_pad_ai2d_input)
            src_pad_ai2d_output = np.ones([1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad], dtype=np.uint8)
            src_pad_ai2d_out_tensor = nn.from_numpy(src_pad_ai2d_output)
            src_pad_ai2d_builder.run(src_pad_ai2d_input_tensor, src_pad_ai2d_out_tensor)

            src_ai2d.set_crop_param(True, int(context_xmin), int(context_ymin), int(context_xmax - context_xmin + 1), int(context_ymax - context_ymin + 1))
            src_ai2d_builder = src_ai2d.build([1, 3, OUT_RGB888P_HEIGHT + top_pad + bottom_pad, OUT_RGB888P_WIDTH + left_pad + right_pad], src_kmodel_input_shape)
            src_ai2d_input_tensor = src_pad_ai2d_out_tensor
            src_ai2d_builder.run(src_ai2d_input_tensor, src_ai2d_out_tensor)
            del src_pad_ai2d_input_tensor
            del src_pad_ai2d_out_tensor
            del src_pad_ai2d_builder
        else:
            src_ai2d.set_crop_param(True, int(center_xy_wh[0] - s_z/2.0), int(center_xy_wh[1] - s_z/2.0), int(s_z), int(s_z))
            src_ai2d_builder = src_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], src_kmodel_input_shape)
            src_ai2d_input = rgb888p_img.to_numpy_ref()
            src_ai2d_input_tensor = nn.from_numpy(src_ai2d_input)
            src_ai2d_builder.run(src_ai2d_input_tensor, src_ai2d_out_tensor)

# 单目标跟踪 对应src模型的 ai2d 释放
def src_ai2d_release():
    with ScopedTiming("src_ai2d_release",debug_mode > 0):
        global src_ai2d_input_tensor,src_ai2d_builder
        del src_ai2d_input_tensor
        del src_ai2d_builder


# 单目标跟踪 crop kpu 初始化
def crop_kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("crop_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)

        crop_ai2d_init()
        return kpu_obj

# 单目标跟踪 crop kpu 输入预处理
def crop_kpu_pre_process(rgb888p_img,center_xy_wh):
    crop_ai2d_run(rgb888p_img,center_xy_wh)
    with ScopedTiming("crop_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,crop_ai2d_out_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, crop_ai2d_out_tensor)

# 单目标跟踪 crop kpu 获取输出
def crop_kpu_get_output():
    with ScopedTiming("crop_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

# 单目标跟踪 crop kpu 运行
def crop_kpu_run(kpu_obj,rgb888p_img,center_xy_wh):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    crop_kpu_pre_process(rgb888p_img,center_xy_wh)
    # (2) kpu 运行
    with ScopedTiming("crop_kpu_run",debug_mode > 0):
        kpu_obj.run()
    # (3) 释放ai2d资源
    crop_ai2d_release()
    # (4) 获取kpu输出
    result = crop_kpu_get_output()
    # 返回 crop kpu 的输出
    return result

# 单目标跟踪 crop kpu 释放
def crop_kpu_deinit():
    with ScopedTiming("crop_kpu_deinit",debug_mode > 0):
        if 'crop_ai2d' in globals():
            global crop_ai2d
            del crop_ai2d
        if 'crop_pad_ai2d' in globals():
            global crop_pad_ai2d
            del crop_pad_ai2d
        if 'crop_ai2d_out_tensor' in globals():
            global crop_ai2d_out_tensor
            del crop_ai2d_out_tensor

# 单目标跟踪 src kpu 初始化
def src_kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("src_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)

        src_ai2d_init()
        return kpu_obj

# 单目标跟踪 src kpu 输入预处理
def src_kpu_pre_process(rgb888p_img,center_xy_wh):
    src_ai2d_run(rgb888p_img,center_xy_wh)
    with ScopedTiming("src_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,src_ai2d_out_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, src_ai2d_out_tensor)

# 单目标跟踪 src kpu 获取输出
def src_kpu_get_output():
    with ScopedTiming("src_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

# 单目标跟踪 src kpu 运行
def src_kpu_run(kpu_obj,rgb888p_img,center_xy_wh):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    src_kpu_pre_process(rgb888p_img,center_xy_wh)
    # (2) kpu 运行
    with ScopedTiming("src_kpu_run",debug_mode > 0):
        kpu_obj.run()
    # (3) 释放ai2d资源
    src_ai2d_release()
    # (4) 获取kpu输出
    result = src_kpu_get_output()
    # 返回 src kpu 的输出
    return result

# 单目标跟踪 src kpu 释放
def src_kpu_deinit():
    with ScopedTiming("src_kpu_deinit",debug_mode > 0):
        if 'src_ai2d' in globals():
            global src_ai2d
            del src_ai2d
        if 'src_pad_ai2d' in globals():
            global src_pad_ai2d
            del src_pad_ai2d
        if 'src_ai2d_out_tensor' in globals():
            global src_ai2d_out_tensor
            del src_ai2d_out_tensor

# 单目标跟踪 track kpu 初始化
def track_kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("track_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)
        return kpu_obj

# 单目标跟踪 track kpu 输入预处理
def track_kpu_pre_process():
    with ScopedTiming("track_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,track_kpu_input_0,track_kpu_input_1
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, track_kpu_input_0)
        current_kmodel_obj.set_input_tensor(1, track_kpu_input_1)

# 单目标跟踪 track kpu 获取输出
def track_kpu_get_output():
    with ScopedTiming("track_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            del data
            results.append(result)
        return results

# 单目标跟踪 track kpu 运行
def track_kpu_run(kpu_obj,center_xy_wh):
    global current_kmodel_obj,track_kpu_input_1
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    track_kpu_pre_process()
    # (2) kpu 运行
    with ScopedTiming("track_kpu_run",debug_mode > 0):
        kpu_obj.run()

    del track_kpu_input_1
    # (4) 获取kpu输出
    results = track_kpu_get_output()
    # (5) track 后处理
    det = track_kpu_post_process(results,center_xy_wh)
    # 返回 跟踪的结果
    return det

# 单目标跟踪 track kpu 释放
def track_kpu_deinit():
    with ScopedTiming("track_kpu_deinit",debug_mode > 0):
        if 'track_kpu_input_0' in globals():
            global track_kpu_input_0
            del track_kpu_input_0



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

#**********for nanotracker.py**********
def nanotracker_inference():
    print("nanotracker start")
    kpu_crop = crop_kpu_init(crop_kmodel_file)                  # 创建单目标跟踪 crop kpu 对象
    kpu_src = src_kpu_init(src_kmodel_file)                     # 创建单目标跟踪 src kpu 对象
    kpu_track = track_kpu_init(track_kmodel_file)               # 创建单目标跟踪 track kpu 对象
    camera_init(CAM_DEV_ID_0)                                   # 初始化 camera
    display_init()                                              # 初始化 display

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)

        run_bool = True
        if (track_x1 < 50 or track_y1 < 50 or track_x1+track_w >= OUT_RGB888P_WIDTH-50 or track_y1+track_h >= OUT_RGB888P_HEIGHT-50):
            print("**剪切范围超出图像范围**")
            run_bool = False

        track_mean_x = track_x1 + track_w / 2.0
        track_mean_y = track_y1 + track_h / 2.0
        draw_mean_w = int(track_w / OUT_RGB888P_WIDTH * DISPLAY_WIDTH)
        draw_mean_h = int(track_h / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT)
        draw_mean_x = int(track_mean_x / OUT_RGB888P_WIDTH * DISPLAY_WIDTH - draw_mean_w / 2.0)
        draw_mean_y = int(track_mean_y / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT - draw_mean_h / 2.0)
        track_w_src = track_w
        track_h_src = track_h

        center_xy_wh = [track_mean_x,track_mean_y,track_w_src,track_h_src]
        center_xy_wh_tmp = [track_mean_x,track_mean_y,track_w_src,track_h_src]

        seconds = 8
        endtime = time.time() + seconds
        enter_init = True

        track_boxes = [track_x1,track_y1,track_w,track_h,1]
        track_boxes_tmp = np.array([track_x1,track_y1,track_w,track_h,1])
        global draw_img,osd_img

        count = 0
        while run_bool:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                rgb888p_img = camera_read(CAM_DEV_ID_0)                 # 读取一帧图片

                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    nowtime = time.time()
                    draw_img.clear()
                    if (enter_init and nowtime <= endtime):
                        print("倒计时: " + str(endtime - nowtime) + " 秒")
                        draw_img.draw_rectangle(draw_mean_x , draw_mean_y , draw_mean_w , draw_mean_h , color=(255, 0, 255, 0),thickness = 4)
                        print(" >>>>>> get trackWindow <<<<<<<<")
                        global track_kpu_input_0
                        track_kpu_input_0 = nn.from_numpy(crop_kpu_run(kpu_crop,rgb888p_img,center_xy_wh))

                        time.sleep(1)
                        if (nowtime > endtime):
                            print(">>>>>>>  Play  <<<<<<<")
                            enter_init = False
                    else:
                        global track_kpu_input_1
                        track_kpu_input_1 = nn.from_numpy(src_kpu_run(kpu_src,rgb888p_img,center_xy_wh))
                        det = track_kpu_run(kpu_track,center_xy_wh)
                        track_boxes = det[0]
                        center_xy_wh = det[1]
                        track_bool = True
                        if (len(track_boxes) != 0):
                            track_bool = track_boxes[0] > 10 and track_boxes[1] > 10 and track_boxes[0] + track_boxes[2] < OUT_RGB888P_WIDTH - 10 and track_boxes[1] + track_boxes[3] < OUT_RGB888P_HEIGHT - 10
                        else:
                            track_bool = False

                        if (len(center_xy_wh) != 0):
                            track_bool = track_bool and center_xy_wh[2] * center_xy_wh[3] < 40000
                        else:
                            track_bool = False

                        if (track_bool):
                            center_xy_wh_tmp = center_xy_wh
                            track_boxes_tmp = track_boxes
                            x1 = int(float(track_boxes[0]) * DISPLAY_WIDTH / OUT_RGB888P_WIDTH)
                            y1 = int(float(track_boxes[1]) * DISPLAY_HEIGHT / OUT_RGB888P_HEIGHT)
                            w = int(float(track_boxes[2]) * DISPLAY_WIDTH / OUT_RGB888P_WIDTH)
                            h = int(float(track_boxes[3]) * DISPLAY_HEIGHT / OUT_RGB888P_HEIGHT)
                            draw_img.draw_rectangle(x1, y1, w, h, color=(255, 255, 0, 0),thickness = 4)
                        else:
                            center_xy_wh = center_xy_wh_tmp
                            track_boxes = track_boxes_tmp
                            x1 = int(float(track_boxes[0]) * DISPLAY_WIDTH / OUT_RGB888P_WIDTH)
                            y1 = int(float(track_boxes[1]) * DISPLAY_HEIGHT / OUT_RGB888P_HEIGHT)
                            w = int(float(track_boxes[2]) * DISPLAY_WIDTH / OUT_RGB888P_WIDTH)
                            h = int(float(track_boxes[3]) * DISPLAY_HEIGHT / OUT_RGB888P_HEIGHT)
                            draw_img.draw_rectangle(x1, y1, w, h, color=(255, 255, 0, 0),thickness = 4)
                            draw_img.draw_string( x1 , y1-50, "Step away from the camera, please !" , color=(255, 255 ,0 , 0), scale=4, thickness = 1)
                            draw_img.draw_string( x1 , y1-100, "Near the center, please !" , color=(255, 255 ,0 , 0), scale=4, thickness = 1)


                    draw_img.copy_to(osd_img)
                    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)

                camera_release_image(CAM_DEV_ID_0,rgb888p_img)                      # camera 释放图像

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

        camera_stop(CAM_DEV_ID_0)                                                   # 停止 camera
        display_deinit()                                                            # 释放 display
        crop_kpu_deinit()                                                           # 释放 单目标跟踪 crop kpu
        src_kpu_deinit()                                                            # 释放 单目标跟踪 src kpu
        track_kpu_deinit()                                                          # 释放 单目标跟踪 track kpu
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_crop
        del kpu_src
        del kpu_track

        gc.collect()
        nn.shrink_memory_pool()
        media_deinit()                                                        # 释放 整个media

    print("nanotracker end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    nanotracker_inference()


