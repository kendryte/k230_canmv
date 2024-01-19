import ulab.numpy as np                  # 类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              # nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *               # 摄像头模块
from media.display import *              # 显示模块
from media.media import *                # 软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import aidemo                            # aidemo模块，封装ai demo相关后处理、画图操作
import image                             # 图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              # 时间统计
import gc                                # 垃圾回收模块
import os, sys                           # 操作系统接口模块
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
# 人脸关键点kmodel输入shape
fld_kmodel_input_shape = (1,3,192,192)
# ai原图padding
rgb_mean = [104,117,123]

#人脸检测kmodel其它参数设置
confidence_threshold = 0.5               # 人脸检测阈值
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
# 人脸关键点kmodel
fr_kmodel_file = root_dir + 'kmodel/face_landmark.kmodel'
# anchor文件
anchors_path = root_dir + 'utils/prior_data_320.bin'
# 调试模型，0：不调试，>0：打印对应级别调试信息
debug_mode = 0

# 人脸关键点不同部位关键点列表
dict_kp_seq = [
    [43, 44, 45, 47, 46, 50, 51, 49, 48],              # left_eyebrow
    [97, 98, 99, 100, 101, 105, 104, 103, 102],        # right_eyebrow
    [35, 36, 33, 37, 39, 42, 40, 41],                  # left_eye
    [89, 90, 87, 91, 93, 96, 94, 95],                  # right_eye
    [34, 88],                                          # pupil
    [72, 73, 74, 86],                                  # bridge_nose
    [77, 78, 79, 80, 85, 84, 83],                      # wing_nose
    [52, 55, 56, 53, 59, 58, 61, 68, 67, 71, 63, 64],  # out_lip
    [65, 54, 60, 57, 69, 70, 62, 66],                  # in_lip
    [1, 9, 10, 11, 12, 13, 14, 15, 16, 2, 3, 4, 5, 6, 7, 8, 0, 24, 23, 22, 21, 20, 19, 18, 32, 31, 30, 29, 28, 27, 26, 25, 17]  # basin
]

# 人脸关键点不同部位（顺序同dict_kp_seq）颜色配置，argb
color_list_for_osd_kp = [
    (255, 0, 255, 0),
    (255, 0, 255, 0),
    (255, 255, 0, 255),
    (255, 255, 0, 255),
    (255, 255, 0, 0),
    (255, 255, 170, 0),
    (255, 255, 255, 0),
    (255, 0, 255, 255),
    (255, 255, 220, 50),
    (255, 30, 30, 255)
]

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
# fld_ai2d：              人脸关键点ai2d实例
# fld_ai2d_input_tensor： 人脸关键点ai2d输入
# fld_ai2d_output_tensor：人脸关键点ai2d输入
# fld_ai2d_builder：      根据人脸关键点ai2d参数，构建的人脸关键点ai2d_builder对象
global fld_ai2d,fld_ai2d_input_tensor,fld_ai2d_output_tensor,fld_ai2d_builder
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
        if 'fd_ai2d' in globals():
            global fd_ai2d
            del fd_ai2d               #删除人脸检测ai2d变量，释放对它所引用对象的内存引用
        if 'fd_ai2d_output_tensor' in globals():
            global fd_ai2d_output_tensor
            del fd_ai2d_output_tensor   #删除人脸检测ai2d_output_tensor变量，释放对它所引用对象的内存引用

###############for face recognition###############
def get_affine_matrix(bbox):
    # 获取仿射矩阵，用于将边界框映射到模型输入空间
    with ScopedTiming("get_affine_matrix", debug_mode > 1):
        # 从边界框提取坐标和尺寸
        x1, y1, w, h = map(lambda x: int(round(x, 0)), bbox[:4])
        # 计算缩放比例，使得边界框映射到模型输入空间的一部分
        scale_ratio = (fld_kmodel_input_shape[2]) / (max(w, h) * 1.5)
        # 计算边界框中心点在模型输入空间的坐标
        cx = (x1 + w / 2) * scale_ratio
        cy = (y1 + h / 2) * scale_ratio
        # 计算模型输入空间的一半长度
        half_input_len = fld_kmodel_input_shape[2] / 2

        # 创建仿射矩阵并进行设置
        matrix_dst = np.zeros((2, 3), dtype=np.float)
        matrix_dst[0, 0] = scale_ratio
        matrix_dst[0, 1] = 0
        matrix_dst[0, 2] = half_input_len - cx
        matrix_dst[1, 0] = 0
        matrix_dst[1, 1] = scale_ratio
        matrix_dst[1, 2] = half_input_len - cy
        return matrix_dst

def fld_ai2d_init():
    # 人脸关键点ai2d初始化
    with ScopedTiming("fld_ai2d_init",debug_mode > 0):
        # （1）创建人脸关键点ai2d对象
        global fld_ai2d
        fld_ai2d = nn.ai2d()

        # （2）创建人脸关键点ai2d_output_tensor对象，用于存放ai2d输出
        global fld_ai2d_output_tensor
        data = np.ones(fld_kmodel_input_shape, dtype=np.uint8)
        fld_ai2d_output_tensor = nn.from_numpy(data)

def fld_ai2d_run(rgb888p_img,det):
    # 人脸关键点ai2d推理
    with ScopedTiming("fld_ai2d_run",debug_mode > 0):
        global fld_ai2d,fld_ai2d_input_tensor,fld_ai2d_output_tensor
        #（1）根据原图ai2d_input_tensor对象
        ai2d_input = rgb888p_img.to_numpy_ref()
        fld_ai2d_input_tensor = nn.from_numpy(ai2d_input)

        # （2）根据新的det设置新的人脸关键点ai2d参数
        fld_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        global matrix_dst
        matrix_dst = get_affine_matrix(det)
        affine_matrix = [matrix_dst[0][0],matrix_dst[0][1],matrix_dst[0][2],
            matrix_dst[1][0],matrix_dst[1][1],matrix_dst[1][2]]
        fld_ai2d.set_affine_param(True,nn.interp_method.cv2_bilinear,0, 0, 127, 1,affine_matrix)

        # （3）根据新的人脸关键点ai2d参数，构建人脸关键点ai2d_builder
        global fld_ai2d_builder
        fld_ai2d_builder = fld_ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], fld_kmodel_input_shape)
        # （4）推理人脸关键点ai2d，将预处理的结果保存到fld_ai2d_output_tensor
        fld_ai2d_builder.run(fld_ai2d_input_tensor, fld_ai2d_output_tensor)

def fld_ai2d_release():
    # 释放人脸关键点ai2d_input_tensor、ai2d_builder
    with ScopedTiming("fld_ai2d_release",debug_mode > 0):
        global fld_ai2d_input_tensor,fld_ai2d_builder
        del fld_ai2d_input_tensor
        del fld_ai2d_builder

def fld_kpu_init(kmodel_file):
    # 人脸关键点kpu初始化
    with ScopedTiming("fld_kpu_init",debug_mode > 0):
        # 初始化人脸关键点kpu对象
        kpu_obj = nn.kpu()
        # 加载人脸关键点kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化人脸关键点ai2d
        fld_ai2d_init()
        return kpu_obj

def fld_kpu_pre_process(rgb888p_img,det):
    # 人脸关键点kpu预处理
    # 人脸关键点ai2d推理，根据det对原图进行预处理
    fld_ai2d_run(rgb888p_img,det)
    with ScopedTiming("fld_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fld_ai2d_output_tensor
        # 将人脸关键点ai2d输出设置为人脸关键点kpu输入
        current_kmodel_obj.set_input_tensor(0, fld_ai2d_output_tensor)
        #ai2d_out_data = fld_ai2d_output_tensor.to_numpy()
        #with open("/sdcard/app/ai2d_out.bin", "wb") as file:
            #file.write(ai2d_out_data.tobytes())

def fld_kpu_get_output():
    with ScopedTiming("fld_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取人脸关键点kpu输出
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

def fld_kpu_post_process(pred):
    # 人脸关键点kpu推理结果后处理
    with ScopedTiming("fld_kpu_post_process",debug_mode > 0):
        # （1）将人脸关键点输出变换模型输入
        half_input_len = fld_kmodel_input_shape[2] // 2
        pred = pred.flatten()
        for i in range(len(pred)):
            pred[i] += (pred[i] + 1) * half_input_len

        # （2）获取仿射矩阵的逆矩阵
        global matrix_dst
        matrix_dst_inv = aidemo.invert_affine_transform(matrix_dst)
        matrix_dst_inv = matrix_dst_inv.flatten()

        # （3）对每个关键点进行逆变换
        half_out_len = len(pred) // 2
        for kp_id in range(half_out_len):
            old_x = pred[kp_id * 2]
            old_y = pred[kp_id * 2 + 1]

            # 逆变换公式
            new_x = old_x * matrix_dst_inv[0] + old_y * matrix_dst_inv[1] + matrix_dst_inv[2]
            new_y = old_x * matrix_dst_inv[3] + old_y * matrix_dst_inv[4] + matrix_dst_inv[5]

            pred[kp_id * 2] = new_x
            pred[kp_id * 2 + 1] = new_y

    return pred

def fld_kpu_run(kpu_obj,rgb888p_img,det):
    # 人脸关键点kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）人脸关键点kpu预处理，设置kpu输入
    fld_kpu_pre_process(rgb888p_img,det)
    # （2）人脸关键点kpu推理
    with ScopedTiming("fld_kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放人脸关键点ai2d
    fld_ai2d_release()
    # （4）获取人脸关键点kpu输出
    result = fld_kpu_get_output()
    # （5）人脸关键点后处理
    result = fld_kpu_post_process(result)
    return result

def fld_kpu_deinit():
    # 人脸关键点kpu释放
    with ScopedTiming("fld_kpu_deinit",debug_mode > 0):
        if 'fld_ai2d' in globals():
            global fld_ai2d
            del fld_ai2d
        if 'fld_ai2d_output_tensor' in globals():
            global fld_ai2d_output_tensor
            del fld_ai2d_output_tensor

#********************for media_utils.py********************
global draw_img_ulab,draw_img,osd_img                       #for display
global buffer,media_source,media_sink                       #for media

# for display，已经封装好，无需自己再实现，直接调用即可，详细解析请查看1.6.2
def display_init():
    # 设置使用hdmi进行显示
    display.init(LT9611_1920X1080_30FPS)
    display.set_plane(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)

def display_deinit():
    # 释放显示资源
    display.deinit()

def display_draw(dets,landmark_preds):
    # 在显示器画人脸轮廓
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img_ulab,draw_img,osd_img
        if dets:
            draw_img.clear()
            for pred in landmark_preds:
                # （1）获取单个人脸框对应的人脸关键点
                for sub_part_index in range(len(dict_kp_seq)):
                    # （2）构建人脸某个区域关键点集
                    sub_part = dict_kp_seq[sub_part_index]
                    face_sub_part_point_set = []
                    for kp_index in range(len(sub_part)):
                        real_kp_index = sub_part[kp_index]
                        x, y = pred[real_kp_index * 2], pred[real_kp_index * 2 + 1]

                        x = int(x * DISPLAY_WIDTH // OUT_RGB888P_WIDTH)
                        y = int(y * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH)
                        face_sub_part_point_set.append((x, y))

                    # （3）画人脸不同区域的轮廓
                    if sub_part_index in (9, 6):
                        color = np.array(color_list_for_osd_kp[sub_part_index],dtype = np.uint8)
                        face_sub_part_point_set = np.array(face_sub_part_point_set)

                        aidemo.polylines(draw_img_ulab, face_sub_part_point_set,False,color,5,8,0)

                    elif sub_part_index == 4:
                        color = color_list_for_osd_kp[sub_part_index]
                        for kp in face_sub_part_point_set:
                            x,y = kp[0],kp[1]
                            draw_img.draw_circle(x,y ,2, color, 1)
                    else:
                        color = np.array(color_list_for_osd_kp[sub_part_index],dtype = np.uint8)
                        face_sub_part_point_set = np.array(face_sub_part_point_set)
                        aidemo.contours(draw_img_ulab, face_sub_part_point_set,-1,color,2,8)

            # （4）将轮廓结果拷贝到osd
            draw_img.copy_to(osd_img)
            # （5）将osd显示到屏幕
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

    media.buffer_config(config)

    global media_source, media_sink
    media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    media_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
    media.create_link(media_source, media_sink)

    # 初始化多媒体buffer
    media.buffer_init()

    global buffer, draw_img_ulab,draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 用于画框
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
def face_landmark_inference():
    print("face_landmark_test start")
    # 人脸检测kpu初始化
    kpu_face_detect = fd_kpu_init(fd_kmodel_file)
    # 人脸关键点kpu初始化
    kpu_face_landmark = fld_kpu_init(fr_kmodel_file)
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
        gc_count = 0
        while True:
            # 设置当前while循环退出点，保证rgb888p_img正确释放
            os.exitpoint()
            with ScopedTiming("total",1):
                # （1）读取一帧图像
                rgb888p_img = camera_read(CAM_DEV_ID_0)
                # （2）若读取成功，推理当前帧
                if rgb888p_img.format() == image.RGBP888:
                    # （2.1）推理当前图像，并获取人脸检测结果
                    dets = fd_kpu_run(kpu_face_detect,rgb888p_img)
                    # （2.2）针对每个人脸框，推理得到对应人脸关键点
                    landmark_result = []
                    for det in dets:
                        ret = fld_kpu_run(kpu_face_landmark,rgb888p_img,det)
                        landmark_result.append(ret)
                    # （2.3）将人脸关键点画到屏幕上
                    display_draw(dets,landmark_result)

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
        # 捕捉运行运行中异常，并打印错误
        sys.print_exception(e)
    finally:
        # 停止camera
        camera_stop(CAM_DEV_ID_0)
        # 释放显示资源
        display_deinit()
        # 释放kpu资源
        fd_kpu_deinit()
        fld_kpu_deinit()
        global current_kmodel_obj
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_face_detect
        del kpu_face_landmark
        nn.shrink_memory_pool()
        if 'draw_img' in globals():
            global draw_img
            del draw_img
        if 'draw_img_ulab' in globals():
            global draw_img_ulab
            del draw_img_ulab
        # 垃圾回收
        gc.collect()
        # 释放媒体资源
        media_deinit()

    print("face_landmark_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    face_landmark_inference()
