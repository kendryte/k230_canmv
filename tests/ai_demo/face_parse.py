import ulab.numpy as np                  # 类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              # nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *               # 摄像头模块
from media.display import *              # 显示模块
from media.media import *                # 软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import aidemo                            # aidemo模块，封装ai demo相关后处理、画图操作
import image                             # 图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              # 时间统计
import gc                                # 垃圾回收模块
import os,sys                            # 操作系统接口模块
import math                              # 数学模块

#********************for config.py********************
# display分辨率
DISPLAY_WIDTH = ALIGN_UP(1920, 16)                   # 显示宽度要求16位对齐
DISPLAY_HEIGHT = 1080

# ai原图分辨率，sensor默认出图为16:9，若需不形变原图，最好按照16:9比例设置宽高
OUT_RGB888P_WIDTH = ALIGN_UP(1920, 16)               # ai原图宽度要求16位对齐
OUT_RGB888P_HEIGH = 1080

# kmodel参数设置
# 人脸检测kmodel输入shape
fd_kmodel_input_shape = (1,3,320,320)
# 人脸解析kmodel输入shape
fp_kmodel_input_shape = (1,3,320,320)
# ai原图padding
rgb_mean = [104,117,123]

#人脸检测kmodel其它参数设置
confidence_threshold = 0.5                      # 人脸检测阈值
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
# 人脸检测kmodel
root_dir = '/sdcard/app/tests/'
fd_kmodel_file = root_dir + 'kmodel/face_detection_320.kmodel'
# 人脸解析kmodel
fp_kmodel_file = root_dir + 'kmodel/face_parse.kmodel'
# anchor文件
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
global current_kmodel_obj #当前kpu对象
# fd_ai2d：               人脸检测ai2d实例
# fd_ai2d_input_tensor：  人脸检测ai2d输入
# fd_ai2d_output_tensor： 人脸检测ai2d输入
# fd_ai2d_builder：       根据人脸检测ai2d参数，构建的人脸检测ai2d_builder对象
global fd_ai2d,fd_ai2d_input_tensor,fd_ai2d_output_tensor,fd_ai2d_builder
# fld_ai2d：              人脸解析ai2d实例
# fld_ai2d_input_tensor： 人脸解析ai2d输入
# fld_ai2d_output_tensor：人脸解析ai2d输入
# fld_ai2d_builder：      根据人脸解析ai2d参数，构建的人脸解析ai2d_builder对象
global fp_ai2d,fp_ai2d_input_tensor,fp_ai2d_output_tensor,fp_ai2d_builder
global matrix_dst         #人脸仿射变换矩阵

#读取anchor文件，为人脸检测后处理做准备
print('anchors_path:',anchors_path)
prior_data = np.fromfile(anchors_path, dtype=np.float)
prior_data = prior_data.reshape((anchor_len,det_dim))

def get_pad_one_side_param():
    # 右padding或下padding，获取padding参数
    dst_w = fd_kmodel_input_shape[3]                         # kmodel输入宽（w）
    dst_h = fd_kmodel_input_shape[2]                          # kmodel输入高（h）

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

def fd_ai2d_init():
    # 人脸检测模型ai2d初始化
    with ScopedTiming("fd_ai2d_init",debug_mode > 0):
        # （1）创建人脸检测ai2d对象
        global fd_ai2d
        fd_ai2d = nn.ai2d()
        # （2）设置人脸检测ai2d参数
        fd_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        fd_ai2d.set_pad_param(True, get_pad_one_side_param(), 0, rgb_mean)
        fd_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)

        #（3）人脸检测ai2d_builder，根据人脸检测ai2d参数、输入输出大小创建ai2d_builder对象
        global fd_ai2d_builder
        fd_ai2d_builder = fd_ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], fd_kmodel_input_shape)

        #（4）创建人脸检测ai2d_output_tensor，用于保存人脸检测ai2d输出
        global fd_ai2d_output_tensor
        data = np.ones(fd_kmodel_input_shape, dtype=np.uint8)
        fd_ai2d_output_tensor = nn.from_numpy(data)

def fd_ai2d_run(rgb888p_img):
    # 根据人脸检测ai2d参数，对原图rgb888p_img进行预处理
    with ScopedTiming("fd_ai2d_run",debug_mode > 0):
        global fd_ai2d_input_tensor,fd_ai2d_output_tensor,fd_ai2d_builder
        # （1）根据原图构建ai2d_input_tensor对象
        ai2d_input = rgb888p_img.to_numpy_ref()
        fd_ai2d_input_tensor = nn.from_numpy(ai2d_input)
        # （2）运行人脸检测ai2d_builder，将结果保存到人脸检测ai2d_output_tensor中
        fd_ai2d_builder.run(fd_ai2d_input_tensor, fd_ai2d_output_tensor)

def fd_ai2d_release():
    # 释放人脸检测ai2d_input_tensor
    with ScopedTiming("fd_ai2d_release",debug_mode > 0):
        global fd_ai2d_input_tensor
        del fd_ai2d_input_tensor


def fd_kpu_init(kmodel_file):
    # 初始化人脸检测kpu对象，并加载kmodel
    with ScopedTiming("fd_kpu_init",debug_mode > 0):
        # 初始化人脸检测kpu对象
        kpu_obj = nn.kpu()
        # 加载人脸检测kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化人脸检测ai2d
        fd_ai2d_init()
        return kpu_obj

def fd_kpu_pre_process(rgb888p_img):
    # 设置人脸检测kpu输入
    # 使用人脸检测ai2d对原图进行预处理（padding，resize）
    fd_ai2d_run(rgb888p_img)
    with ScopedTiming("fd_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fd_ai2d_output_tensor
        # 设置人脸检测kpu输入
        current_kmodel_obj.set_input_tensor(0, fd_ai2d_output_tensor)

def fd_kpu_get_output():
    # 获取人脸检测kpu输出
    with ScopedTiming("fd_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取模型输出，并将结果转换为numpy，以便进行人脸检测后处理
        results = []
        for i in range(current_kmodel_obj.outputs_size()):
            data = current_kmodel_obj.get_output_tensor(i)
            result = data.to_numpy()
            del data
            results.append(result)
        return results

def fd_kpu_run(kpu_obj,rgb888p_img):
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）原图预处理，并设置模型输入
    fd_kpu_pre_process(rgb888p_img)
    # （2）人脸检测kpu推理
    with ScopedTiming("fd kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放人脸检测ai2d资源
    fd_ai2d_release()
    # （4）获取人俩检测kpu输出
    results = fd_kpu_get_output()
    # （5）人脸检测kpu结果后处理
    with ScopedTiming("fd kpu_post",debug_mode > 0):
        post_ret = aidemo.face_det_post_process(confidence_threshold,nms_threshold,fd_kmodel_input_shape[2],prior_data,
                [OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGH],results)
    # （6）返回人脸检测框
    if len(post_ret)==0:
        return post_ret
    else:
        return post_ret[0]          #0:det,1:landm,2:score

def fd_kpu_deinit():
    # kpu释放
    with ScopedTiming("fd_kpu_deinit",debug_mode > 0):
        if 'fd_ai2d' in globals():     #删除人脸检测ai2d变量，释放对它所引用对象的内存引用
            global fd_ai2d
            del fd_ai2d
        if 'fd_ai2d_output_tensor' in globals():#删除人脸检测ai2d_output_tensor变量，释放对它所引用对象的内存引用
            global fd_ai2d_output_tensor
            del fd_ai2d_output_tensor

###############for face recognition###############
def get_affine_matrix(bbox):
    # 获取仿射矩阵，用于将边界框映射到模型输入空间
    with ScopedTiming("get_affine_matrix", debug_mode > 1):
        # 设置缩放因子
        factor = 2.7
        # 从边界框提取坐标和尺寸
        x1, y1, w, h = map(lambda x: int(round(x, 0)), bbox[:4])
        # 模型输入大小
        edge_size = fp_kmodel_input_shape[2]
        # 平移距离，使得模型输入空间的中心对准原点
        trans_distance = edge_size / 2.0
        # 计算边界框中心点的坐标
        center_x = x1 + w / 2.0
        center_y = y1 + h / 2.0
        # 计算最大边长
        maximum_edge = factor * (h if h > w else w)
        # 计算缩放比例
        scale = edge_size * 2.0 / maximum_edge
        # 计算平移参数
        cx = trans_distance - scale * center_x
        cy = trans_distance - scale * center_y
        # 创建仿射矩阵
        affine_matrix = [scale, 0, cx, 0, scale, cy]
        return affine_matrix

def fp_ai2d_init():
    # 人脸解析ai2d初始化
    with ScopedTiming("fp_ai2d_init",debug_mode > 0):
        # （1）创建人脸解析ai2d对象
        global fp_ai2d
        fp_ai2d = nn.ai2d()

        # （2）创建人脸解析ai2d_output_tensor对象
        global fp_ai2d_output_tensor
        data = np.ones(fp_kmodel_input_shape, dtype=np.uint8)
        fp_ai2d_output_tensor = nn.from_numpy(data)

def fp_ai2d_run(rgb888p_img,det):
    # 人脸解析ai2d推理
    with ScopedTiming("fp_ai2d_run",debug_mode > 0):
        global fp_ai2d,fp_ai2d_input_tensor,fp_ai2d_output_tensor
        #（1）根据原图构建人脸解析ai2d_input_tensor
        ai2d_input = rgb888p_img.to_numpy_ref()
        fp_ai2d_input_tensor = nn.from_numpy(ai2d_input)
        #（2）设置人脸解析ai2d参数
        fp_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        global matrix_dst
        matrix_dst = get_affine_matrix(det)
        fp_ai2d.set_affine_param(True,nn.interp_method.cv2_bilinear,0, 0, 127, 1,matrix_dst)

        # （3）构建人脸解析ai2d_builder
        global fp_ai2d_builder
        fp_ai2d_builder = fp_ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], fp_kmodel_input_shape)
        # （4）推理人脸解析ai2d，将结果保存到ai2d_output_tensor
        fp_ai2d_builder.run(fp_ai2d_input_tensor, fp_ai2d_output_tensor)

def fp_ai2d_release():
    # 释放部分人脸解析ai2d资源
    with ScopedTiming("fp_ai2d_release",debug_mode > 0):
        global fp_ai2d_input_tensor,fp_ai2d_builder
        del fp_ai2d_input_tensor
        del fp_ai2d_builder

def fp_kpu_init(kmodel_file):
    # 初始化人脸解析kpu及ai2d
    with ScopedTiming("fp_kpu_init",debug_mode > 0):
        kpu_obj = nn.kpu()
        kpu_obj.load_kmodel(kmodel_file)
        fp_ai2d_init()
        return kpu_obj

def fp_kpu_pre_process(rgb888p_img,det):
    # 人脸解析kpu预处理
    fp_ai2d_run(rgb888p_img,det)
    with ScopedTiming("fp_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fp_ai2d_output_tensor
        current_kmodel_obj.set_input_tensor(0, fp_ai2d_output_tensor)
        #ai2d_out_data = fp_ai2d_output_tensor.to_numpy()
        #with open("/sdcard/app/ai2d_out.bin", "wb") as file:
            #file.write(ai2d_out_data.tobytes())

def fp_kpu_get_output():
    # 获取人脸解析kpu输出
    with ScopedTiming("fp_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

def fp_kpu_run(kpu_obj,rgb888p_img,det):
    # 人脸解析kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）根据人脸检测框进行人脸解析kpu预处理
    fp_kpu_pre_process(rgb888p_img,det)
    # （2）人脸解析kpu推理
    with ScopedTiming("fp_kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放人脸解析ai2d资源
    fp_ai2d_release()
    # （4）释放人脸解析kpu输出
    result = fp_kpu_get_output()
    return result

def fp_kpu_deinit():
    # 释放人脸解析kpu和ai2d资源
    with ScopedTiming("fp_kpu_deinit",debug_mode > 0):
        if 'fp_ai2d' in globals():
            global fp_ai2d
            del fp_ai2d
        if 'fp_ai2d_output_tensor' in globals():
            global fp_ai2d_output_tensor
            del fp_ai2d_output_tensor

#********************for media_utils.py********************
global draw_img_ulab,draw_img,osd_img                                     #for display
global buffer,media_source,media_sink                       #for media

#for display
def display_init():
    # 设置使用hdmi进行显示
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

def display_deinit():
    # 释放显示资源
    display.deinit()

def display_draw(dets,parse_results):
    # 在显示器画出人脸解析结果
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img_ulab,draw_img,osd_img
        if dets:
            draw_img.clear()
            for i,det in enumerate(dets):
                # （1）将人脸检测框画到draw_img
                x, y, w, h = map(lambda x: int(round(x, 0)), det[:4])
                x = x * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                y = y * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                w = w * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                h = h * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
                draw_img.draw_rectangle(x,y, w, h, color=(255, 255, 0, 255))
                # （2）将人脸解析结果画到draw_img（draw_img_ulab和draw_img指同一内存）
                aidemo.face_parse_post_process(draw_img_ulab,[OUT_RGB888P_WIDTH,OUT_RGB888P_HEIGH],
                    [DISPLAY_WIDTH,DISPLAY_HEIGHT],fp_kmodel_input_shape[2],det.tolist(),parse_results[i])
            # （3）将绘制好的图像拷贝到显示缓冲区，并在显示器上展示
            draw_img.copy_to(osd_img)
            display.show_image(osd_img, 0, 0, DISPLAY_CHN_OSD3)
        else:
            # （1）清空用来画框的图像
            draw_img.clear()
            # （2）清空osd
            draw_img.copy_to(osd_img)
            # （3）显示透明图层
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

    # 初始化多媒体buffer
    media.buffer_init()

    global buffer, draw_img_ulab,draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 用于画框，draw_img->draw_img_ulab(两者指向同一块内存)
    draw_img_ulab = np.zeros((DISPLAY_HEIGHT,DISPLAY_WIDTH,4),dtype=np.uint8)
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, alloc=image.ALLOC_REF,data = draw_img_ulab)
    # 用于拷贝画框结果，防止画框过程中发生buffer搬运
    osd_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888, poolid=buffer.pool_id, alloc=image.ALLOC_VB,
                          phyaddr=buffer.phys_addr, virtaddr=buffer.virt_addr)

def media_deinit():
    # meida资源释放
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    if 'buffer' in globals():
        global buffer
        media.release_buffer(buffer)

    if 'media_source' in globals() and 'media_sink' in globals():
        global media_source, media_sink
        media.destroy_link(media_source, media_sink)

    media.buffer_deinit()

#********************for face_detect.py********************
def face_parse_inference():
    print("face_parse_test start")
    # 人脸检测kpu初始化
    kpu_face_detect = fd_kpu_init(fd_kmodel_file)
    # 人脸解析kpu初始化
    kpu_face_parse = fp_kpu_init(fp_kmodel_file)
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
        while True:
            os.exitpoint()
            with ScopedTiming("total",1):
                # （1）读取一帧图像
                rgb888p_img = camera_read(CAM_DEV_ID_0)

                # （2）若读取成功，推理当前帧
                if rgb888p_img.format() == image.RGBP888:
                    # （2.1）推理当前图像，并获取人脸检测结果
                    dets = fd_kpu_run(kpu_face_detect,rgb888p_img)
                    # （2.2）针对每个人脸框，推理得到对应人脸解析结果
                    parse_results = []
                    for det in dets:
                        parse_ret = fp_kpu_run(kpu_face_parse,rgb888p_img,det)
                        parse_results.append(parse_ret)
                    # （2.3）将人脸解析结果画到显示器上
                    display_draw(dets,parse_results)

                # （3）释放当前帧
                camera_release_image(CAM_DEV_ID_0,rgb888p_img)
                gc.collect()
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
        fd_kpu_deinit()
        fp_kpu_deinit()
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_face_detect
        del kpu_face_parse
        if 'draw_img' in globals():
            global draw_img
            del draw_img
        if 'draw_img_ulab' in globals():
            global draw_img_ulab
            del draw_img_ulab
        # 垃圾回收
        gc.collect()
        nn.shrink_memory_pool()
        # 释放媒体资源
        media_deinit()

    print("face_parse_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    face_parse_inference()
