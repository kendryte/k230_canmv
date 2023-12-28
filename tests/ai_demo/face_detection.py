import ulab.numpy as np                  #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *               #摄像头模块
from media.display import *              #显示模块
from media.media import *                #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import aidemo                            #aidemo模块，封装ai demo相关后处理、画图操作
import image                             #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              #时间统计
import gc                                #垃圾回收模块

#********************for config.py********************
# display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)                   # 显示宽度要求16位对齐
DISPLAY_HEIGHT = 1080

# ai原图分辨率，sensor默认出图为16:9，若需不形变原图，最好按照16:9比例设置宽高
OUT_RGB888P_WIDTH = ALIGN_UP(1920, 16)               # ai原图宽度要求16位对齐
OUT_RGB888P_HEIGH = 1080

# kmodel参数设置
# kmodel输入shape
kmodel_input_shape = (1,3,320,320)
# ai原图padding
rgb_mean = [104,117,123]
# kmodel其它参数设置
confidence_threshold = 0.5
top_k = 5000
nms_threshold = 0.2
keep_top_k = 750
vis_thres = 0.5
variance = [0.1, 0.2]
anchor_len = 4200
score_dim = 2
det_dim = 4
keypoint_dim = 10

# 文件配置
# kmodel文件配置
root_dir = '/sdcard/app/tests/'
kmodel_file = root_dir + 'kmodel/face_detection_320.kmodel'
# anchor文件配置
anchors_path = root_dir + 'utils/prior_data_320.bin'
# 调试模型，0：不调试，>0：打印对应级别调试信息
debug_mode = 0

#********************for scoped_timing.py********************
# 时间统计类
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

#********************for ai_utils.py********************
# 当前kmodel
global current_kmodel_obj
# ai2d：              ai2d实例
# ai2d_input_tensor： ai2d输入
# ai2d_output_tensor：ai2d输出
# ai2d_builder：      根据ai2d参数，构建的ai2d_builder对象
global ai2d,ai2d_input_tensor,ai2d_output_tensor,ai2d_builder    #for ai2d
print('anchors_path:',anchors_path)
# 读取anchor文件，为后处理做准备
prior_data = np.fromfile(anchors_path, dtype=np.float)
prior_data = prior_data.reshape((anchor_len,det_dim))

def get_pad_one_side_param():
    # 右padding或下padding，获取padding参数
    dst_w = kmodel_input_shape[3]                         # kmodel输入宽（w）
    dst_h = kmodel_input_shape[2]                          # kmodel输入高（h）

    # OUT_RGB888P_WIDTH：原图宽（w）
    # OUT_RGB888P_HEIGH：原图高（h）
    # 计算最小的缩放比例，等比例缩放
    ratio_w = dst_w / OUT_RGB888P_WIDTH
    ratio_h = dst_h / OUT_RGB888P_HEIGH
    if ratio_w < ratio_h:
        ratio = ratio_w
    else:
        ratio = ratio_h
    # 计算经过缩放后的新宽和新高
    new_w = (int)(ratio * OUT_RGB888P_WIDTH)
    new_h = (int)(ratio * OUT_RGB888P_HEIGH)

    # 计算需要添加的padding，以使得kmodel输入的宽高和原图一致
    dw = (dst_w - new_w) / 2
    dh = (dst_h - new_h) / 2
    # 四舍五入，确保padding是整数
    top = (int)(round(0))
    bottom = (int)(round(dh * 2 + 0.1))
    left = (int)(round(0))
    right = (int)(round(dw * 2 - 0.1))
    return [0, 0, 0, 0, top, bottom, left, right]

def ai2d_init():
    # 人脸检测模型ai2d初始化
    with ScopedTiming("ai2d_init",debug_mode > 0):
        # （1）创建ai2d对象
        global ai2d
        ai2d = nn.ai2d()
        # （2）设置ai2d参数
        ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        ai2d.set_pad_param(True, get_pad_one_side_param(), 0, rgb_mean)
        ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        # （3）创建ai2d_output_tensor，用于保存ai2d输出
        global ai2d_output_tensor
        data = np.ones(kmodel_input_shape, dtype=np.uint8)
        ai2d_output_tensor = nn.from_numpy(data)

        # （4）ai2d_builder，根据ai2d参数、输入输出大小创建ai2d_builder对象
        global ai2d_builder
        ai2d_builder = ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], kmodel_input_shape)


def ai2d_run(rgb888p_img):
    # 对原图rgb888p_img进行预处理
    with ScopedTiming("ai2d_run",debug_mode > 0):
        global ai2d_input_tensor,ai2d_output_tensor
        # （1）根据原图构建ai2d_input_tensor对象
        ai2d_input = rgb888p_img.to_numpy_ref()
        ai2d_input_tensor = nn.from_numpy(ai2d_input)
        # （2）运行ai2d_builder，将结果保存到ai2d_output_tensor中
        ai2d_builder.run(ai2d_input_tensor, ai2d_output_tensor)

def ai2d_release():
    # 释放ai2d_input_tensor
    with ScopedTiming("ai2d_release",debug_mode > 0):
        global ai2d_input_tensor
        del ai2d_input_tensor

def kpu_init(kmodel_file):
    # 初始化kpu对象，并加载kmodel
    with ScopedTiming("kpu_init",debug_mode > 0):
        # 初始化kpu对象
        kpu_obj = nn.kpu()
        # 加载kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化ai2d
        ai2d_init()
        return kpu_obj

def kpu_pre_process(rgb888p_img):
    # 使用ai2d对原图进行预处理（padding，resize）
    ai2d_run(rgb888p_img)
    with ScopedTiming("kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,ai2d_output_tensor
        # 将ai2d输出设置为kpu输入
        current_kmodel_obj.set_input_tensor(0, ai2d_output_tensor)

def kpu_get_output():
    with ScopedTiming("kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取模型输出，并将结果转换为numpy，以便进行人脸检测后处理
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            del data
            results.append(result)
        return results

def kpu_run(kpu_obj,rgb888p_img):
    # kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）原图预处理，并设置模型输入
    kpu_pre_process(rgb888p_img)
    # （2）kpu推理
    with ScopedTiming("kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放ai2d资源
    ai2d_release()
    # （4）获取kpu输出
    results = kpu_get_output()
    # （5）kpu结果后处理
    with ScopedTiming("kpu_post",debug_mode > 0):
        post_ret = aidemo.face_det_post_process(confidence_threshold,nms_threshold,kmodel_input_shape[2],prior_data,[OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGH],results)

    # （6）返回人脸检测框
    if len(post_ret)==0:
        return post_ret
    else:
        return post_ret[0]


def kpu_deinit():
    # kpu释放
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        global ai2d,ai2d_output_tensor
        del ai2d                      #删除ai2d变量，释放对它所引用对象的内存引用
        del ai2d_output_tensor        #删除ai2d_output_tensor变量，释放对它所引用对象的内存引用

#********************for media_utils.py********************
global draw_img,osd_img                                     #for display
global buffer,media_source,media_sink                       #for media

# for display，已经封装好，无需自己再实现，直接调用即可，详细解析请查看1.6.2
def display_init():
    # hdmi显示初始化
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

def display_deinit():
    # 释放显示资源
    display.deinit()

def display_draw(dets):
    # hdmi画检测框
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img
        if dets:
            draw_img.clear()
            for det in dets:
                x, y, w, h = map(lambda x: int(round(x, 0)), det[:4])
                x = x * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                y = y * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
                w = w * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                h = h * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
                draw_img.draw_rectangle(x,y, w, h, color=(255, 255, 0, 255), thickness = 2)
            draw_img.copy_to(osd_img)
            display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)
        else:
            draw_img.clear()
            draw_img.copy_to(osd_img)
            display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)

#for camera，已经封装好，无需自己再实现，直接调用即可，详细解析请查看1.6.1
def camera_init(dev_id):
    # camera初始化
    camera.sensor_init(dev_id, CAM_DEFAULT_SENSOR)

    # set chn0 output yuv420sp
    camera.set_outsize(dev_id, CAM_CHN_ID_0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    camera.set_outfmt(dev_id, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    # set chn2 output rgb88planar
    camera.set_outsize(dev_id, CAM_CHN_ID_2, OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGH)
    camera.set_outfmt(dev_id, CAM_CHN_ID_2, PIXEL_FORMAT_RGB_888_PLANAR)

def camera_start(dev_id):
    # camera启动
    camera.start_stream(dev_id)

def camera_read(dev_id):
    # 读取一帧图像
    with ScopedTiming("camera_read",debug_mode >0):
        rgb888p_img = camera.capture_image(dev_id, CAM_CHN_ID_2)
        return rgb888p_img

def camera_release_image(dev_id,rgb888p_img):
    # 释放一帧图像
    with ScopedTiming("camera_release_image",debug_mode >0):
        camera.release_image(dev_id, CAM_CHN_ID_2, rgb888p_img)

def camera_stop(dev_id):
    # 停止camera
    camera.stop_stream(dev_id)

#for media，已经封装好，无需自己再实现，直接调用即可，详细解析请查看1.6.3
def media_init():
    # meida初始化
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

    # 初始化媒体buffer
    ret = media.buffer_init()
    if ret:
        return ret
    global buffer, draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 用于画框
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, alloc=image.ALLOC_MPGC)
    # 用于拷贝画框结果，防止画框过程中发生buffer搬运
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, poolid=buffer.pool_id, alloc=image.ALLOC_VB,
                          phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr)
    return ret

def media_deinit():
    # meida资源释放
    global buffer,media_source, media_sink
    media.release_buffer(buffer)
    media.destroy_link(media_source, media_sink)

    ret = media.buffer_deinit()
    return ret


#********************for face_detect.py********************
def face_detect_inference():
    print("face_detect_test start")
    # kpu初始化
    kpu_face_detect = kpu_init(kmodel_file)
    # camera初始化
    camera_init(CAM_DEV_ID_0)
    # 显示初始化
    display_init()

    rgb888p_img = None
    # 注意：将一定要将一下过程包在try中，用于保证程序停止后，资源释放完毕；确保下次程序仍能正常运行
    try:
        # 注意：媒体初始化（注：媒体初始化必须在camera_start之前，确保media缓冲区已配置完全）
        ret = media_init()
        if ret:
            print("face_detect_test, buffer init failed")
            return ret
        # 启动camera
        camera_start(CAM_DEV_ID_0)
        time.sleep(5)
        gc_count = 0
        while True:
            with ScopedTiming("total",1):
                # （1）读取一帧图像
                rgb888p_img = camera_read(CAM_DEV_ID_0)
                # （2）若读取失败，释放当前帧
                if rgb888p_img == -1:
                    print("face_detect_test, capture_image failed")
                    camera_release_image(CAM_DEV_ID_0,rgb888p_img)
                    rgb888p_img = None
                    continue

                # （3）若读取成功，推理当前帧
                if rgb888p_img.format() == image.RGBP888:
                    # （3.1）推理当前图像，并获取检测结果
                    dets = kpu_run(kpu_face_detect,rgb888p_img)
                    # （3.2）将结果画到显示器
                    display_draw(dets)

                # （4）释放当前帧
                camera_release_image(CAM_DEV_ID_0,rgb888p_img)
                rgb888p_img = None
                if gc_count > 5:
                    gc.collect()
                    gc_count = 0
                else:
                    gc_count += 1
    except Exception as e:
        # 捕捉运行运行中异常，并打印错误
        print(f"An error occurred during buffer used: {e}")
    finally:
        # 释放当前帧
        if rgb888p_img is not None:
            #先release掉申请的内存再stop
            camera_release_image(CAM_DEV_ID_0,rgb888p_img)

        # 停止camera
        camera_stop(CAM_DEV_ID_0)
        # 释放显示资源
        display_deinit()
        # 释放kpu资源
        kpu_deinit()
        global current_kmodel_obj
        del current_kmodel_obj
        del kpu_face_detect
        # 垃圾回收
        gc.collect()
        time.sleep(1)
        # 释放媒体资源
        ret = media_deinit()
        if ret:
            print("face_detect_test, buffer_deinit failed")
            return ret

    print("face_detect_test end")
    return 0

if __name__ == '__main__':
    face_detect_inference()
