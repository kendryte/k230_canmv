import ulab.numpy as np             #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn         #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *          #摄像头模块
from media.display import *         #显示模块
from media.media import *           #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import image                        #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                         #时间统计
import gc                           #垃圾回收模块
import os,sys                       #操作系统接口模块
import aicube                       #aicube模块，封装检测分割等任务相关后处理

# display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)
DISPLAY_HEIGHT = 1080

# ai原图分辨率输入
OUT_RGB888P_WIDTH = ALIGN_UP(640, 16)
OUT_RGB888P_HEIGH = 360

# kmodel输入参数配置
kmodel_input_shape_det = (1,3,640,640)      # kmodel输入分辨率
rgb_mean = [0,0,0]                          # ai2d padding的值

# kmodel相关参数设置
mask_threshold = 0.25                       # 二值化mask阈值
box_threshold = 0.3                         # 检测框分数阈值

# 文件配置
root_dir = '/sdcard/app/tests/'
kmodel_file_det = root_dir + 'kmodel/ocr_det_int16.kmodel'    # kmodel加载路径
debug_mode = 0                                                # 调试模式 大于0（调试）、 反之 （不调试）

# scoped_timing.py 用于debug模式输出程序块运行时间
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

# ai utils
# 当前kmodel
global current_kmodel_obj                                                                  # 定义全局kpu对象
# ai2d_det: ai2d实例
# ai2d_input_tensor_det: ai2d输入
# ai2d_output_tensor_det: ai2d输出
# ai2d_builder_det: 根据ai2d参数，构建的ai2d_builder_det对象
# ai2d_input_det: ai2d输入的numpy数据
global ai2d_det,ai2d_input_tensor_det,ai2d_output_tensor_det,ai2d_builder_det,ai2d_input_det    # 定义全局 ai2d 对象，并且定义 ai2d 的输入、输出 以及 builder

# padding方法，一边padding，右padding或者下padding
def get_pad_one_side_param(out_img_size,input_img_size):
    # 右padding或下padding
    dst_w = out_img_size[0]
    dst_h = out_img_size[1]

    input_width = input_img_size[0]
    input_high = input_img_size[1]

    ratio_w = dst_w / input_width
    ratio_h = dst_h / input_high
    if ratio_w < ratio_h:
        ratio = ratio_w
    else:
        ratio = ratio_h

    new_w = (int)(ratio * input_width)
    new_h = (int)(ratio * input_high)
    dw = (dst_w - new_w) / 2
    dh = (dst_h - new_h) / 2

    top = (int)(round(0))
    bottom = (int)(round(dh * 2 + 0.1))
    left = (int)(round(0))
    right = (int)(round(dw * 2 - 0.1))
    return [0, 0, 0, 0, top, bottom, left, right]


# ai2d 初始化，用于实现输入的预处理
def ai2d_init_det():
    with ScopedTiming("ai2d_init",debug_mode > 0):
        # 创建ai2d对象
        global ai2d_det
        ai2d_det = nn.ai2d()
        # 设置ai2d参数
        ai2d_det.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        ai2d_det.set_pad_param(True, get_pad_one_side_param([kmodel_input_shape_det[3],kmodel_input_shape_det[2]], [OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGH]), 0, [0, 0, 0])
        ai2d_det.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
        # 创建ai2d_output_tensor_det，用于保存ai2d的输出
        global ai2d_output_tensor_det
        data = np.ones(kmodel_input_shape_det, dtype=np.uint8)
        ai2d_output_tensor_det = nn.from_numpy(data)

        # ai2d_builder_det，根据ai2d参数、输入输出大小创建ai2d_builder_det对象
        global ai2d_builder_det
        ai2d_builder_det = ai2d_det.build([1, 3, OUT_RGB888P_HEIGH, OUT_RGB888P_WIDTH], [1, 3, kmodel_input_shape_det[2], kmodel_input_shape_det[3]])


# ai2d 运行，完成ai2d_init_det设定的预处理
def ai2d_run_det(rgb888p_img):
    # 对原图rgb888p_img进行预处理
    with ScopedTiming("ai2d_run",debug_mode > 0):
        # 根据原图构建ai2d_input_tensor_det
        global ai2d_input_tensor_det,ai2d_builder_det,ai2d_input_det,ai2d_output_tensor_det
        ai2d_input_det = rgb888p_img.to_numpy_ref()
        ai2d_input_tensor_det = nn.from_numpy(ai2d_input_det)
        # 运行ai2d_builder_det，将结果保存到ai2d_output_tensor_det
        ai2d_builder_det.run(ai2d_input_tensor_det, ai2d_output_tensor_det)


# ai2d 释放输入tensor
def ai2d_release_det():
    with ScopedTiming("ai2d_release",debug_mode > 0):
        global ai2d_input_tensor_det
        del ai2d_input_tensor_det

# kpu 初始化
def kpu_init_det(kmodel_file):
    # 初始化kpu对象，并加载kmodel
    with ScopedTiming("kpu_init",debug_mode > 0):
        # 初始化kpu对象
        kpu_obj = nn.kpu()
        # 加载kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化ai2d
        ai2d_init_det()
        return kpu_obj

# 预处理方法
def kpu_pre_process_det(rgb888p_img):
    # 运行ai2d，将ai2d预处理的输出设置为kmodel的输入tensor
    ai2d_run_det(rgb888p_img)
    with ScopedTiming("kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,ai2d_output_tensor_det
        # 将ai2d的输出设置为kmodel的输入
        current_kmodel_obj.set_input_tensor(0, ai2d_output_tensor_det)

# 获取kmodel的推理输出
def kpu_get_output():
    with ScopedTiming("kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        results = []
        # 获取模型输出，并将结果转换为numpy,以便后续处理
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            del data
            results.append(result)
        return results

# kpu 运行
def kpu_run_det(kpu_obj,rgb888p_img):
    # kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    #（1）原图像预处理并设置模型输入
    kpu_pre_process_det(rgb888p_img)
    #（2）kpu推理
    with ScopedTiming("kpu_run",debug_mode > 0):
        kpu_obj.run()
    #（3）释放ai2d资源
    ai2d_release_det()
    #（4）获取kpu输出
    results = kpu_get_output()
    #（5）CHW转HWC
    global ai2d_input_det
    tmp = (ai2d_input_det.shape[0], ai2d_input_det.shape[1], ai2d_input_det.shape[2])
    ai2d_input_det = ai2d_input_det.reshape((ai2d_input_det.shape[0], ai2d_input_det.shape[1] * ai2d_input_det.shape[2]))
    ai2d_input_det = ai2d_input_det.transpose()
    tmp2 = ai2d_input_det.copy()
    tmp2 = tmp2.reshape((tmp[1], tmp[2], tmp[0]))
    #（6）后处理，aicube.ocr_post_process接口说明：
    #  接口：aicube.ocr_post_process(threshold_map,ai_isp,kmodel_input_shape,isp_shape,mask_threshold,box_threshold);
    #  参数说明：
    #     threshold_map: DBNet模型的输出为（N,kmodel_input_shape_det[2],kmodel_input_shape_det[3],2），两个通道分别为threshold map和segmentation map
    #     后处理过程只使用threshold map，因此将results[0][:,:,:,0] reshape成一维传给接口使用。
    #     ai_isp：后处理还会返回基于原图的检测框裁剪数据，因此要将原图数据reshape为一维传给接口处理。
    #     kmodel_input_shape：kmodel输入分辨率。
    #     isp_shape：AI原图分辨率。要将kmodel输出分辨率的检测框坐标映射到原图分辨率上，需要使用这两个分辨率的值。
    #     mask_threshold：用于二值化图像获得文本区域。
    #     box_threshold：检测框分数阈值，低于该阈值的检测框不计入结果。
    with ScopedTiming("kpu_post",debug_mode > 0):
        # 调用aicube模块的ocr_post_process完成ocr检测的后处理
        # det_results结构为[[crop_array_nhwc,[p1_x,p1_y,p2_x,p2_y,p3_x,p3_y,p4_x,p4_y]],...]
        det_results = aicube.ocr_post_process(results[0][:, :, :, 0].reshape(-1), tmp2.reshape(-1),
                                                  [kmodel_input_shape_det[3], kmodel_input_shape_det[2]],
                                                  [OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGH], mask_threshold, box_threshold)
    return det_results


# kpu 释放内存
def kpu_deinit_det():
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        global ai2d_det,ai2d_output_tensor_det,ai2d_input_tensor_det
        if "ai2d" in globals():
            del ai2d_det
        if "ai2d_output_tensor_det" in globals():
            del ai2d_output_tensor_det

#********************for media_utils.py********************

global draw_img,osd_img                                     #for display
global buffer,media_source,media_sink                       #for media

#display 初始化
def display_init():
    # hdmi显示初始化
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

# display 释放内存
def display_deinit():
    display.deinit()

# display 作图过程，将OCR检测后处理得到的框绘制到OSD上并显示
def display_draw(det_results):
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img
        if det_results:
            draw_img.clear()
            # 循环绘制所有检测到的框
            for j in det_results:
                # 将原图的坐标点转换成显示的坐标点，循环绘制四条直线，得到一个矩形框
                for i in range(4):
                    x1 = j[1][(i * 2)] / OUT_RGB888P_WIDTH * DISPLAY_WIDTH
                    y1 = j[1][(i * 2 + 1)] / OUT_RGB888P_HEIGH * DISPLAY_HEIGHT
                    x2 = j[1][((i + 1) * 2) % 8] / OUT_RGB888P_WIDTH * DISPLAY_WIDTH
                    y2 = j[1][((i + 1) * 2 + 1) % 8] / OUT_RGB888P_HEIGH * DISPLAY_HEIGHT
                    draw_img.draw_line((int(x1), int(y1), int(x2), int(y2)), color=(255, 0, 0, 255),
                                       thickness=5)
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
    camera.set_outsize(dev_id, CAM_CHN_ID_2, OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGH)
    camera.set_outfmt(dev_id, CAM_CHN_ID_2, PIXEL_FORMAT_RGB_888_PLANAR)

# camera 启动视频流
def camera_start(dev_id):
    camera.start_stream(dev_id)

# camera 捕获一帧图像
def camera_read(dev_id):
    with ScopedTiming("camera_read",debug_mode >0):
        rgb888p_img = camera.capture_image(dev_id, CAM_CHN_ID_2)
        return rgb888p_img

# camera 释放内存
def camera_release_image(dev_id,rgb888p_img):
    with ScopedTiming("camera_release_image",debug_mode >0):
        camera.release_image(dev_id, CAM_CHN_ID_2, rgb888p_img)

# 停止视频流
def camera_stop(dev_id):
    camera.stop_stream(dev_id)

#for media 初始化
def media_init():
    config = k_vb_config()
    config.max_pool_cnt = 1
    config.comm_pool[0].blk_size = 4 * DISPLAY_WIDTH * DISPLAY_HEIGHT
    config.comm_pool[0].blk_cnt = 1
    config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE

    ret = media.buffer_config(config)

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
    return ret

# media 释放buffer，销毁link
def media_deinit():
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    global buffer,media_source, media_sink
    if "buffer" in globals():
        media.release_buffer(buffer)
    if 'media_source' in globals() and 'media_sink' in globals():
        media.destroy_link(media_source, media_sink)
    media.buffer_deinit()


def ocr_det_inference():
    print("ocr_det_test start")
    kpu_ocr_det = kpu_init_det(kmodel_file_det)     # 创建ocr检测任务的kpu对象
    camera_init(CAM_DEV_ID_0)                       # 初始化 camera
    display_init()                                  # 初始化 display

    # 注意：将一定要将一下过程包在try中，用于保证程序停止后，资源释放完毕；确保下次程序仍能正常运行
    try:
        # 注意：媒体初始化（注：媒体初始化必须在camera_start之前，确保media缓冲区已配置完全）
        media_init()
        # 启动camera
        camera_start(CAM_DEV_ID_0)
        gc_count=0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                # 读取一帧图像
                rgb888p_img = camera_read(CAM_DEV_ID_0) # 读取一帧图像
                # 若图像获取成功，推理当前帧
                if rgb888p_img.format() == image.RGBP888:
                    det_results = kpu_run_det(kpu_ocr_det,rgb888p_img)  # kpu运行获取kmodel的推理输出
                    display_draw(det_results)                           # 绘制检测结果，并显示
                # 释放当前帧
                camera_release_image(CAM_DEV_ID_0,rgb888p_img)          # 释放内存
                if (gc_count>2):
                    gc.collect()
                    gc_count = 0
                else:
                    gc_count += 1
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        camera_stop(CAM_DEV_ID_0)                                       # 停止camera
        display_deinit()                                                # 释放display
        kpu_deinit_det()                                                # 释放kpu
        if "current_kmodel_obj" in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_ocr_det
        gc.collect()
        nn.shrink_memory_pool()
        time.sleep(1)
        media_deinit()                                                  # 释放整个media
    print("ocr_det_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    ocr_det_inference()
