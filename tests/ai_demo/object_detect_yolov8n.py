import ulab.numpy as np             #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn         #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *          #摄像头模块
from media.display import *         #显示模块
from media.media import *           #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import image                        #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                         #时间统计
import gc                           #垃圾回收模块
import os, sys                      #操作系统接口模块

##config.py
#display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

#ai 原图输入分辨率
OUT_RGB888P_WIDTH = ALIGN_UP(320, 16)
OUT_RGB888P_HEIGHT = 320

#多目标检测 kmodel 输入 shape
kmodel_input_shape = (1,3,320,320)

#多目标检测 相关参数设置
confidence_threshold = 0.2                                      # 多目标检测分数阈值
nms_threshold = 0.2                                             # 非最大值抑制阈值
x_factor = float(OUT_RGB888P_WIDTH)/kmodel_input_shape[3]       # 原始图像分辨率宽与kmodel宽输入大小比值
y_factor = float(OUT_RGB888P_HEIGHT)/kmodel_input_shape[2]      # 原始图像分辨率高与kmodel高输入大小比值
keep_top_k = 50                                                 # 最大输出检测框的数量

#文件配置
root_dir = '/sdcard/app/tests/'
kmodel_file = root_dir + 'kmodel/yolov8n_320.kmodel'           # kmodel文件的路径
debug_mode = 0                                                  # debug模式 大于0（调试）、 反之 （不调试）

#颜色板 用于作图
color_four = [(255, 220, 20, 60), (255, 119, 11, 32), (255, 0, 0, 142), (255, 0, 0, 230),
        (255, 106, 0, 228), (255, 0, 60, 100), (255, 0, 80, 100), (255, 0, 0, 70),
        (255, 0, 0, 192), (255, 250, 170, 30), (255, 100, 170, 30), (255, 220, 220, 0),
        (255, 175, 116, 175), (255, 250, 0, 30), (255, 165, 42, 42), (255, 255, 77, 255),
        (255, 0, 226, 252), (255, 182, 182, 255), (255, 0, 82, 0), (255, 120, 166, 157),
        (255, 110, 76, 0), (255, 174, 57, 255), (255, 199, 100, 0), (255, 72, 0, 118),
        (255, 255, 179, 240), (255, 0, 125, 92), (255, 209, 0, 151), (255, 188, 208, 182),
        (255, 0, 220, 176), (255, 255, 99, 164), (255, 92, 0, 73), (255, 133, 129, 255),
        (255, 78, 180, 255), (255, 0, 228, 0), (255, 174, 255, 243), (255, 45, 89, 255),
        (255, 134, 134, 103), (255, 145, 148, 174), (255, 255, 208, 186),
        (255, 197, 226, 255), (255, 171, 134, 1), (255, 109, 63, 54), (255, 207, 138, 255),
        (255, 151, 0, 95), (255, 9, 80, 61), (255, 84, 105, 51), (255, 74, 65, 105),
        (255, 166, 196, 102), (255, 208, 195, 210), (255, 255, 109, 65), (255, 0, 143, 149),
        (255, 179, 0, 194), (255, 209, 99, 106), (255, 5, 121, 0), (255, 227, 255, 205),
        (255, 147, 186, 208), (255, 153, 69, 1), (255, 3, 95, 161), (255, 163, 255, 0),
        (255, 119, 0, 170), (255, 0, 182, 199), (255, 0, 165, 120), (255, 183, 130, 88),
        (255, 95, 32, 0), (255, 130, 114, 135), (255, 110, 129, 133), (255, 166, 74, 118),
        (255, 219, 142, 185), (255, 79, 210, 114), (255, 178, 90, 62), (255, 65, 70, 15),
        (255, 127, 167, 115), (255, 59, 105, 106), (255, 142, 108, 45), (255, 196, 172, 0),
        (255, 95, 54, 80), (255, 128, 76, 255), (255, 201, 57, 1), (255, 246, 0, 122),
        (255, 191, 162, 208)]

#标签 多目标检测的所有可识别类别
labels = ["person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"]

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
global current_kmodel_obj                                           # 定义全局的 kpu 对象
global ai2d,ai2d_input_tensor,ai2d_output_tensor,ai2d_builder       # 定义全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder


# 多目标检测 非最大值抑制方法实现
def py_cpu_nms(boxes,scores,thresh):
    """Pure Python NMS baseline."""
    x1 = boxes[:, 0]
    y1 = boxes[:, 1]
    x2 = boxes[:, 2]
    y2 = boxes[:, 3]

    areas = (x2 - x1 + 1) * (y2 - y1 + 1)
    order = np.argsort(scores,axis = 0)[::-1]

    keep = []
    while order.size > 0:
        i = order[0]
        keep.append(i)
        new_x1 = []
        new_x2 = []
        new_y1 = []
        new_y2 = []
        new_areas = []
        for order_i in order:
            new_x1.append(x1[order_i])
            new_x2.append(x2[order_i])
            new_y1.append(y1[order_i])
            new_y2.append(y2[order_i])
            new_areas.append(areas[order_i])
        new_x1 = np.array(new_x1)
        new_x2 = np.array(new_x2)
        new_y1 = np.array(new_y1)
        new_y2 = np.array(new_y2)
        xx1 = np.maximum(x1[i], new_x1)
        yy1 = np.maximum(y1[i], new_y1)
        xx2 = np.minimum(x2[i], new_x2)
        yy2 = np.minimum(y2[i], new_y2)

        w = np.maximum(0.0, xx2 - xx1 + 1)
        h = np.maximum(0.0, yy2 - yy1 + 1)
        inter = w * h

        new_areas = np.array(new_areas)
        ovr = inter / (areas[i] + new_areas - inter)
        new_order = []
        for ovr_i,ind in enumerate(ovr):
            if ind < thresh:
                new_order.append(order[ovr_i])
        order = np.array(new_order,dtype=np.uint8)
    return keep

# 多目标检测 接收kmodel输出的后处理方法
def kpu_post_process(output_data):
    with ScopedTiming("kpu_post_process", debug_mode > 0):
        boxes_ori = output_data[:,0:4]
        scores_ori = output_data[:,4:]
        confs_ori = np.max(scores_ori,axis=-1)
        inds_ori = np.argmax(scores_ori,axis=-1)

        boxes = []
        scores = []
        inds = []

        for i in range(len(boxes_ori)):
            if confs_ori[i] > confidence_threshold:
                scores.append(confs_ori[i])
                inds.append(inds_ori[i])
                x = boxes_ori[i,0]
                y = boxes_ori[i,1]
                w = boxes_ori[i,2]
                h = boxes_ori[i,3]
                left = int((x - 0.5 * w) * x_factor)
                top = int((y - 0.5 * h) * y_factor)
                right = int((x + 0.5 * w) * x_factor)
                bottom = int((y + 0.5 * h) * y_factor)
                boxes.append([left,top,right,bottom])

        if len(boxes)==0:
            return []

        boxes = np.array(boxes)
        scores = np.array(scores)
        inds = np.array(inds)

        # do NMS
        keep = py_cpu_nms(boxes,scores,nms_threshold)
        dets = np.concatenate((boxes, scores.reshape((len(boxes),1)), inds.reshape((len(boxes),1))), axis=1)

        dets_out = []
        for keep_i in keep:
            dets_out.append(dets[keep_i])
        dets_out = np.array(dets_out)

        # keep top-K faster NMS
        dets_out = dets_out[:keep_top_k, :]
        return dets_out

# ai2d 初始化
def ai2d_init():
    with ScopedTiming("ai2d_init",debug_mode > 0):
        global ai2d
        ai2d = nn.ai2d()
        ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        global ai2d_out_tensor
        data = np.ones(kmodel_input_shape, dtype=np.uint8)
        ai2d_out_tensor = nn.from_numpy(data)

        global ai2d_builder
        ai2d_builder = ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], kmodel_input_shape)

# ai2d 运行
def ai2d_run(rgb888p_img):
    with ScopedTiming("ai2d_run",debug_mode > 0):
        global ai2d_input_tensor,ai2d_out_tensor,ai2d_builder
        ai2d_input = rgb888p_img.to_numpy_ref()
        ai2d_input_tensor = nn.from_numpy(ai2d_input)

        ai2d_builder.run(ai2d_input_tensor, ai2d_out_tensor)

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
        global current_kmodel_obj,ai2d_out_tensor
        # set kpu input
        current_kmodel_obj.set_input_tensor(0, ai2d_out_tensor)

# kpu 获得 kmodel 输出
def kpu_get_output():
    with ScopedTiming("kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()

        result = result.reshape((result.shape[0] * result.shape[1], result.shape[2]))
        result = result.transpose()
        tmp2 = result.copy()
        del data
        results.append(tmp2)
        return results

# kpu 运行
def kpu_run(kpu_obj,rgb888p_img):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # (1) 原图预处理，并设置模型输入
    kpu_pre_process(rgb888p_img)
    # (2) kpu运行
    with ScopedTiming("kpu_run",debug_mode > 0):
        kpu_obj.run()
    # (3) 释放ai2d资源
    ai2d_release()
    # (4) 获取kpu输出
    results = kpu_get_output()
    # (5) kpu结果后处理
    dets = kpu_post_process(results[0])
    # (6) 返回多目标检测结果
    return dets

# kpu 释放内存
def kpu_deinit():
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        if 'ai2d' in globals():
            global ai2d
            del ai2d
        if 'ai2d_out_tensor' in globals():
            global ai2d_out_tensor
            del ai2d_out_tensor
        if 'ai2d_builder' in globals():
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

# display 作图过程 将所有目标检测框以及类别、分数值的作图
def display_draw(dets):
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img
        if dets:
            draw_img.clear()
            for det in dets:
                x1, y1, x2, y2 = map(lambda x: int(round(x, 0)), det[:4])
                w = (x2 - x1) * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                h = (y2 - y1) * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT
                draw_img.draw_rectangle(x1 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH,
                                        y1 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT, w, h, color=color_four[int(det[5])],thickness=4)
                draw_img.draw_string( int(x1 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH) , int(y1 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGHT)-50,
                                         " " + labels[int(det[5])] + " " + str(round(det[4],2)) , color=color_four[int(det[5])] , scale=4)
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


#**********for ob_detect.py**********
def ob_detect_inference():
    print("ob_detect start")
    kpu_ob_detect = kpu_init(kmodel_file)                           # 创建多目标检测的 kpu 对象
    camera_init(CAM_DEV_ID_0)                                       # 初始化 camera
    display_init()                                                  # 初始化 display

    try:
        media_init()

        camera_start(CAM_DEV_ID_0)

        count = 0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                rgb888p_img = camera_read(CAM_DEV_ID_0)             # 读取一帧图片

                # for rgb888planar
                if rgb888p_img.format() == image.RGBP888:
                    dets = kpu_run(kpu_ob_detect,rgb888p_img)       # 执行多目标检测 kpu运行 以及 后处理过程
                    display_draw(dets)                              # 将得到的检测结果 绘制到 display

                camera_release_image(CAM_DEV_ID_0,rgb888p_img)      # camera 释放图像

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

        camera_stop(CAM_DEV_ID_0)                                   # 停止camera
        display_deinit()                                            # 释放 display
        kpu_deinit()                                                # 释放 kpu

        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_ob_detect

        gc.collect()
        nn.shrink_memory_pool()
        media_deinit()                                        # 释放 整个media

    print("ob_detect_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    ob_detect_inference()
