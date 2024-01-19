import ulab.numpy as np                  #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *               #摄像头模块
from media.display import *              #显示模块
from media.media import *                #软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import aidemo                            #aidemo模块，封装ai demo相关后处理、画图操作
import image                             #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              #时间统计
import gc                                #垃圾回收模块
import os                                #基本的操作系统交互功能
import os, sys                           #操作系统接口模块

#********************for config.py********************
# display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)                   # 显示宽度要求16位对齐
DISPLAY_HEIGHT = 1080

# ai原图分辨率，sensor默认出图为16:9，若需不形变原图，最好按照16:9比例设置宽高
OUT_RGB888P_WIDTH = ALIGN_UP(1920, 16)               # ai原图宽度要求16位对齐
OUT_RGB888P_HEIGHT = 1080

# kmodel参数设置
# kmodel输入shape
kmodel_input_shape = (1,3,224,224)
# kmodel其它参数设置
crop_w = 400                                         #图像剪切范围w
crop_h = 400                                         #图像剪切范围h
crop_x = OUT_RGB888P_WIDTH / 2.0 - crop_w / 2.0      #图像剪切范围x
crop_y = OUT_RGB888P_HEIGHT / 2.0 - crop_h / 2.0     #图像剪切范围y
thres = 0.5                                          #特征判别阈值
top_k = 3                                            #识别范围
categories = ['apple','banana']                      #识别类别
features = [2,2]                                     #对应类别注册特征数量
time_one = 100                                       #注册单个特征中途间隔帧数

# 文件配置
# kmodel文件配置
root_dir = '/sdcard/app/tests/'
kmodel_file = root_dir + 'kmodel/recognition.kmodel'
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

# 获取两个特征向量的相似度
def getSimilarity(output_vec,save_vec):
    tmp = sum(output_vec * save_vec)
    mold_out = np.sqrt(sum(output_vec * output_vec))
    mold_save = np.sqrt(sum(save_vec * save_vec))
    return tmp / (mold_out * mold_save)

# 自学习 ai2d 初始化
def ai2d_init():
    with ScopedTiming("ai2d_init",debug_mode > 0):
        global ai2d
        ai2d = nn.ai2d()
        global ai2d_output_tensor
        data = np.ones(kmodel_input_shape, dtype=np.uint8)
        ai2d_output_tensor = nn.from_numpy(data)

# 自学习 ai2d 运行
def ai2d_run(rgb888p_img):
    with ScopedTiming("ai2d_run",debug_mode > 0):
        global ai2d,ai2d_input_tensor,ai2d_output_tensor
        ai2d_input = rgb888p_img.to_numpy_ref()
        ai2d_input_tensor = nn.from_numpy(ai2d_input)

        ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)

        ai2d.set_crop_param(True,int(crop_x),int(crop_y),int(crop_w),int(crop_h))
        ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        global ai2d_builder
        ai2d_builder = ai2d.build([1,3,OUT_RGB888P_HEIGHT,OUT_RGB888P_WIDTH], kmodel_input_shape)
        ai2d_builder.run(ai2d_input_tensor, ai2d_output_tensor)

# 自学习 ai2d 释放
def ai2d_release():
    with ScopedTiming("ai2d_release",debug_mode > 0):
        global ai2d_input_tensor,ai2d_builder
        del ai2d_input_tensor
        del ai2d_builder

# 自学习 kpu 初始化
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

# 自学习 kpu 输入预处理
def kpu_pre_process(rgb888p_img):
    # 使用ai2d对原图进行预处理（crop，resize）
    ai2d_run(rgb888p_img)
    with ScopedTiming("kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,ai2d_output_tensor
        # 将ai2d输出设置为kpu输入
        current_kmodel_obj.set_input_tensor(0, ai2d_output_tensor)

# 自学习 kpu 获取输出
def kpu_get_output():
    with ScopedTiming("kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取模型输出，并将结果转换为numpy，以便进行人脸检测后处理
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            result = result.reshape(-1)
            del data
            results.append(result)
        return results

# 自学习 kpu 运行
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

    # （5）返回输出
    return results

# 自学习 kpu 释放
def kpu_deinit():
    # kpu释放
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        if 'ai2d' in globals():
            global ai2d
            del ai2d
        if 'ai2d_output_tensor' in globals():
            global ai2d_output_tensor
            del ai2d_output_tensor

#********************for media_utils.py********************
global draw_img,osd_img                                     #for display
global buffer,media_source,media_sink                       #for media

# for display，已经封装好，无需自己再实现，直接调用即可
def display_init():
    # hdmi显示初始化
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

def display_deinit():
    # 释放显示资源
    display.deinit()

#for camera，已经封装好，无需自己再实现，直接调用即可
def camera_init(dev_id):
    # camera初始化
    camera.sensor_init(dev_id, CAM_DEFAULT_SENSOR)

    # set chn0 output yuv420sp
    camera.set_outsize(dev_id, CAM_CHN_ID_0, DISPLAY_WIDTH, DISPLAY_HEIGHT)
    camera.set_outfmt(dev_id, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    # set chn2 output rgb88planar
    camera.set_outsize(dev_id, CAM_CHN_ID_2, OUT_RGB888P_WIDTH, OUT_RGB888P_HEIGHT)
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

#for media，已经封装好，无需自己再实现，直接调用即可
def media_init():
    # meida初始化
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

    # 初始化媒体buffer
    media.buffer_init()

    global buffer, draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 用于画框
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
    # 用于拷贝画框结果，防止画框过程中发生buffer搬运
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, poolid=buffer.pool_id, alloc=image.ALLOC_VB,
                          phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr)

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

#********************for self_learning.py********************
def self_learning_inference():
    print("self_learning_test start")
    # kpu初始化
    kpu_self_learning = kpu_init(kmodel_file)
    # camera初始化
    camera_init(CAM_DEV_ID_0)
    # 显示初始化
    display_init()

    # 注意：将一定要将一下过程包在try中，用于保证程序停止后，资源释放完毕；确保下次程序仍能正常运行
    try:
        # 注意：媒体初始化（注：媒体初始化必须在camera_start之前，确保media缓冲区已配置完全）
        media_init()

        # 启动camera
        camera_start(CAM_DEV_ID_0)

        crop_x_osd = int(crop_x / OUT_RGB888P_WIDTH * DISPLAY_WIDTH)
        crop_y_osd = int(crop_y / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT)
        crop_w_osd = int(crop_w / OUT_RGB888P_WIDTH * DISPLAY_WIDTH)
        crop_h_osd = int(crop_h / OUT_RGB888P_HEIGHT * DISPLAY_HEIGHT)

#        stat_info = os.stat(root_dir + 'utils/features')
#        if (not (stat_info[0] & 0x4000)):
#            os.mkdir(root_dir + 'utils/features')

        time_all = 0
        time_now = 0
        category_index = 0
        for i in range(len(categories)):
            for j in range(features[i]):
                time_all += time_one

        gc_count = 0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                # （1）读取一帧图像
                rgb888p_img = camera_read(CAM_DEV_ID_0)

                # （2）若读取成功，推理当前帧
                if rgb888p_img.format() == image.RGBP888:
                    # （2.1）推理当前图像，并获取检测结果
                    results = kpu_run(kpu_self_learning,rgb888p_img)
                    global draw_img, osd_img
                    draw_img.clear()
                    draw_img.draw_rectangle(crop_x_osd,crop_y_osd, crop_w_osd, crop_h_osd, color=(255, 255, 0, 255), thickness = 4)

                    if (category_index < len(categories)):
                        time_now += 1
                        draw_img.draw_string( 50 , 200, categories[category_index] + "_" + str(int(time_now-1) // time_one) + ".bin", color=(255,255,0,0), scale=7)
                        with open(root_dir + 'utils/features/' + categories[category_index] + "_" + str(int(time_now-1) // time_one) + ".bin", 'wb') as f:
                            f.write(results[0].tobytes())
                        if (time_now // time_one == features[category_index]):
                            category_index += 1
                            time_all -= time_now
                            time_now = 0
                    else:
                        results_learn = []
                        list_features = os.listdir(root_dir + 'utils/features/')
                        for feature in list_features:
                            with open(root_dir + 'utils/features/' + feature, 'rb') as f:
                                data = f.read()
                            save_vec = np.frombuffer(data, dtype=np.float)
                            score = getSimilarity(results[0], save_vec)

                            if (score > thres):
                                res = feature.split("_")
                                is_same = False
                                for r in results_learn:
                                    if (r["category"] ==  res[0]):
                                        if (r["score"] < score):
                                            r["bin_file"] = feature
                                            r["score"] = score
                                        is_same = True

                                if (not is_same):
                                    if(len(results_learn) < top_k):
                                        evec = {}
                                        evec["category"] = res[0]
                                        evec["score"] = score
                                        evec["bin_file"] = feature
                                        results_learn.append( evec )
                                        results_learn = sorted(results_learn, key=lambda x: -x["score"])
                                    else:
                                        if( score <= results_learn[top_k-1]["score"] ):
                                            continue
                                        else:
                                            evec = {}
                                            evec["category"] = res[0]
                                            evec["score"] = score
                                            evec["bin_file"] = feature
                                            results_learn.append( evec )
                                            results_learn = sorted(results_learn, key=lambda x: -x["score"])

                                            results_learn.pop()
                        draw_y = 200
                        for r in results_learn:
                            draw_img.draw_string( 50 , draw_y, r["category"] + " : " + str(r["score"]), color=(255,255,0,0), scale=7)
                            draw_y += 50

                    draw_img.copy_to(osd_img)
                    display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)

                # （3）释放当前帧
                camera_release_image(CAM_DEV_ID_0,rgb888p_img)

                if gc_count > 5:
                    gc.collect()
                    gc_count = 0
                else:
                    gc_count += 1
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:

        # 停止camera
        camera_stop(CAM_DEV_ID_0)
        # 释放显示资源
        display_deinit()
        # 释放kpu资源
        kpu_deinit()
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_self_learning
        # 删除features文件夹
        stat_info = os.stat(root_dir + 'utils/features')
        if (stat_info[0] & 0x4000):
            list_files = os.listdir(root_dir + 'utils/features')
            for l in list_files:
                os.remove(root_dir + 'utils/features/' + l)
        # 垃圾回收
        gc.collect()
        # 释放媒体资源
        nn.shrink_memory_pool()
        media_deinit()

    print("self_learning_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    self_learning_inference()
