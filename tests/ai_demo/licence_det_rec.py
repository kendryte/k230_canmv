import ulab.numpy as np             #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn         #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *          #摄像头模块
from media.display import *         #显示模块
from media.media import *           #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import image                        #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                         #时间统计
import gc                           #垃圾回收模块
import aidemo                       #aidemo模块，封装ai demo相关后处理、画图操作
import os, sys                           #操作系统接口模块

##config.py
#display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

#ai原图分辨率输入
OUT_RGB888P_WIDTH = ALIGN_UP(640, 16)
OUT_RGB888P_HEIGHT = 360

#车牌检测 和 车牌识别 kmodel输入shape
det_kmodel_input_shape = (1,3,640,640)
rec_kmodel_input_shape = (1,1,32,220)

#车牌检测 相关参数设置
obj_thresh = 0.2            #车牌检测分数阈值
nms_thresh = 0.2            #检测框 非极大值抑制 阈值

#文件配置
root_dir = '/sdcard/app/tests/'
det_kmodel_file = root_dir + 'kmodel/LPD_640.kmodel'                               # 车牌检测 kmodel 文件路径
rec_kmodel_file = root_dir + 'kmodel/licence_reco.kmodel'                          # 车牌识别 kmodel 文件路径
#dict_rec = ["挂", "使", "领", "澳", "港", "皖", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉", "黑", "苏", "浙", "京", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁", "新", "警", "学", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "_", "-"]
dict_rec = ["gua","shi","ling","ao","gang","wan","hu","jin","yu","ji","jin","meng","liao","ji","hei","su","zhe","jing","min","gan","lu","yu","e","xiang","yue","gui","qiong","chuan","gui","yun","zang","shan","gan","qing","ning","xin","jing","xue","0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "_", "-"]
dict_size = len(dict_rec)
debug_mode = 0                                                                      # debug模式 大于0（调试）、 反之 （不调试）

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
global current_kmodel_obj                                                           # 定义全局的 kpu 对象
global det_ai2d,det_ai2d_input_tensor,det_ai2d_output_tensor,det_ai2d_builder       # 定义车牌检测 ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder
global rec_ai2d,rec_ai2d_input_tensor,rec_ai2d_output_tensor,rec_ai2d_builder       # 定义车牌识别 ai2d 对象 ，并且定义 ai2d 的输入、输出 以及 builder

# 车牌检测 接收kmodel输出的后处理方法
def det_kpu_post_process(output_data):
    with ScopedTiming("det_kpu_post_process", debug_mode > 0):
        results = aidemo.licence_det_postprocess(output_data,[OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH],[det_kmodel_input_shape[2],det_kmodel_input_shape[3]],obj_thresh,nms_thresh)
        return results

# 车牌识别 接收kmodel输出的后处理方法
def rec_kpu_post_process(output_data):
    with ScopedTiming("rec_kpu_post_process", debug_mode > 0):
        size = rec_kmodel_input_shape[3] / 4
        result = []
        for i in range(size):
            maxs = float("-inf")
            index = -1
            for j in range(dict_size):
                if (maxs < float(output_data[i * dict_size +j])):
                    index = j
                    maxs = output_data[i * dict_size +j]
            result.append(index)

        result_str = ""
        for i in range(size):
            if (result[i] >= 0 and result[i] != 0 and not(i > 0 and result[i-1] == result[i])):
                result_str += dict_rec[result[i]-1]
        return result_str

# 车牌检测 ai2d 初始化
def det_ai2d_init():
    with ScopedTiming("det_ai2d_init",debug_mode > 0):
        global det_ai2d
        det_ai2d = nn.ai2d()
        det_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        det_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        global det_ai2d_out_tensor
        data = np.ones(det_kmodel_input_shape, dtype=np.uint8)
        det_ai2d_out_tensor = nn.from_numpy(data)

        global det_ai2d_builder
        det_ai2d_builder = det_ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], det_kmodel_input_shape)

# 车牌识别 ai2d 初始化
def rec_ai2d_init():
    with ScopedTiming("rec_ai2d_init",debug_mode > 0):
        global rec_ai2d
        rec_ai2d = nn.ai2d()
        rec_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)

        global rec_ai2d_out_tensor
        data = np.ones(rec_kmodel_input_shape, dtype=np.uint8)
        rec_ai2d_out_tensor = nn.from_numpy(data)

        rec_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

# 车牌检测 ai2d 运行
def det_ai2d_run(rgb888p_img):
    with ScopedTiming("det_ai2d_run",debug_mode > 0):
        global det_ai2d_input_tensor,det_ai2d_out_tensor,det_ai2d_builder
        det_ai2d_input = rgb888p_img.to_numpy_ref()
        det_ai2d_input_tensor = nn.from_numpy(det_ai2d_input)

        det_ai2d_builder.run(det_ai2d_input_tensor, det_ai2d_out_tensor)

# 车牌识别 ai2d 运行
def rec_ai2d_run(img_array):
    with ScopedTiming("rec_ai2d_run",debug_mode > 0):
        global rec_ai2d_input_tensor,rec_ai2d_out_tensor,rec_ai2d_builder
        rec_ai2d_builder = rec_ai2d.build([1,1,img_array.shape[2],img_array.shape[3]], rec_kmodel_input_shape)
        rec_ai2d_input_tensor = nn.from_numpy(img_array)

        rec_ai2d_builder.run(rec_ai2d_input_tensor, rec_ai2d_out_tensor)

# 车牌检测 ai2d 释放内存
def det_ai2d_release():
    with ScopedTiming("det_ai2d_release",debug_mode > 0):
        global det_ai2d_input_tensor
        del det_ai2d_input_tensor

# 车牌识别 ai2d 释放内存
def rec_ai2d_release():
    with ScopedTiming("rec_ai2d_release",debug_mode > 0):
        global rec_ai2d_input_tensor, rec_ai2d_builder
        del rec_ai2d_input_tensor
        del rec_ai2d_builder

# 车牌检测 kpu 初始化
def det_kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("det_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)

        det_ai2d_init()
        return kpu_obj

# 车牌识别 kpu 初始化
def rec_kpu_init(kmodel_file):
    # init kpu and load kmodel
    with ScopedTiming("rec_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)

        rec_ai2d_init()
        return kpu_obj

# 车牌检测 kpu 输入预处理
def det_kpu_pre_process(rgb888p_img):
    det_ai2d_run(rgb888p_img)
    with ScopedTiming("det_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,det_ai2d_out_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, det_ai2d_out_tensor)

# 车牌识别 kpu 输入预处理
def rec_kpu_pre_process(img_array):
    rec_ai2d_run(img_array)
    with ScopedTiming("rec_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,rec_ai2d_out_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, rec_ai2d_out_tensor)

# 车牌识别 抠图
def rec_array_pre_process(rgb888p_img,dets):
    with ScopedTiming("rec_array_pre_process",debug_mode > 0):
        isp_image = rgb888p_img.to_numpy_ref()
        imgs_array_boxes = aidemo.ocr_rec_preprocess(isp_image,[OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH],dets)
    return imgs_array_boxes

# 车牌检测 获取 kmodel 输出
def det_kpu_get_output():
    with ScopedTiming("det_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            tmp2 = result.copy()
            del data
            results.append(tmp2)
        return results

# 车牌识别 获取 kmodel 输出
def rec_kpu_get_output():
    with ScopedTiming("rec_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        result = result.reshape((result.shape[0] * result.shape[1] * result.shape[2]))
        tmp = result.copy()
        del data
        return tmp

# 车牌检测 kpu 运行
def det_kpu_run(kpu_obj,rgb888p_img):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    det_kpu_pre_process(rgb888p_img)
    # (2) kpu 运行
    with ScopedTiming("det_kpu_run",debug_mode > 0):
        kpu_obj.run()
     # (3) 释放ai2d资源
    det_ai2d_release()
    # (4) 获取kpu输出
    results = det_kpu_get_output()
    # (5) kpu结果后处理
    dets = det_kpu_post_process(results)
    # 返回 车牌检测结果
    return dets

# 车牌识别 kpu 运行
def rec_kpu_run(kpu_obj,rgb888p_img,dets):
    global current_kmodel_obj
    if (len(dets) == 0):
        return []
    current_kmodel_obj = kpu_obj
    # (1) 原始图像抠图，车牌检测结果 points 排序
    imgs_array_boxes = rec_array_pre_process(rgb888p_img,dets)
    imgs_array = imgs_array_boxes[0]
    boxes = imgs_array_boxes[1]
    recs = []
    for img_array in imgs_array:
        # (2) 抠出后的图像 进行预处理，设置模型输入
        rec_kpu_pre_process(img_array)
        # (3) kpu 运行
        with ScopedTiming("rec_kpu_run",debug_mode > 0):
            kpu_obj.run()
        # (4) 释放ai2d资源
        rec_ai2d_release()
        # (5) 获取 kpu 输出
        result = rec_kpu_get_output()
        # (6) kpu 结果后处理
        rec = rec_kpu_post_process(result)
        recs.append(rec)
    # (7) 返回 车牌检测 和 识别结果
    return [boxes,recs]


# 车牌检测 kpu 释放内存
def det_kpu_deinit():
    with ScopedTiming("det_kpu_deinit",debug_mode > 0):
        if 'det_ai2d' in globals():
            global det_ai2d
            del det_ai2d
        if 'det_ai2d_builder' in globals():
            global det_ai2d_builder
            del det_ai2d_builder
        if 'det_ai2d_out_tensor' in globals():
            global det_ai2d_out_tensor
            del det_ai2d_out_tensor

# 车牌识别 kpu 释放内存
def rec_kpu_deinit():
    with ScopedTiming("rec_kpu_deinit",debug_mode > 0):
        if 'rec_ai2d' in globals():
            global rec_ai2d
            del rec_ai2d
        if 'rec_ai2d_out_tensor' in globals():
            global rec_ai2d_out_tensor
            del rec_ai2d_out_tensor


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

# display 作图过程 将所有车牌检测框 和 识别结果绘制到屏幕
def display_draw(dets_recs):
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img
        if dets_recs:
            dets = dets_recs[0]
            recs = dets_recs[1]
            draw_img.clear()
            point_8 = np.zeros((8),dtype=np.int16)
            for det_index in range(len(dets)):
                for i in range(4):
                    x = dets[det_index][i * 2 + 0]/OUT_RGB888P_WIDTH*DISPLAY_WIDTH
                    y = dets[det_index][i * 2 + 1]/OUT_RGB888P_HEIGHT*DISPLAY_HEIGHT
                    point_8[i * 2 + 0] = int(x)
                    point_8[i * 2 + 1] = int(y)
                for i in range(4):
                    draw_img.draw_line(point_8[i * 2 + 0],point_8[i * 2 + 1],point_8[(i+1) % 4 * 2 + 0],point_8[(i+1) % 4 * 2 + 1],color=(255, 0, 255, 0),thickness=4)
                draw_img.draw_string( point_8[6], point_8[7] + 20, recs[det_index] , color=(255,255,153,18) , scale=4)
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


#**********for licence_det_rec.py**********
def licence_det_rec_inference():
    print("licence_det_rec start")
    kpu_licence_det = det_kpu_init(det_kmodel_file)                     # 创建车牌检测的 kpu 对象
    kpu_licence_rec = rec_kpu_init(rec_kmodel_file)                     # 创建车牌识别的 kpu 对象
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
                    dets = det_kpu_run(kpu_licence_det,rgb888p_img)                 # 执行车牌检测 kpu 运行 以及 后处理过程
                    dets_recs = rec_kpu_run(kpu_licence_rec,rgb888p_img,dets)       # 执行车牌识别 kpu 运行 以及 后处理过程
                    display_draw(dets_recs)                                         # 将得到的检测结果和识别结果 绘制到display

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
        det_kpu_deinit()                                                            # 释放 车牌检测 kpu
        rec_kpu_deinit()                                                            # 释放 车牌识别 kpu
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_licence_det
        del kpu_licence_rec
        gc.collect()
        nn.shrink_memory_pool()
        media_deinit()                                                        # 释放 整个media

    print("licence_det_rec end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    licence_det_rec_inference()


