import ulab.numpy as np                  # 类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              # nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
from media.camera import *               # 摄像头模块
from media.display import *              # 显示模块
from media.media import *                # 软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import aidemo                            # aidemo模块，封装ai demo相关后处理、画图操作
import image                             # 图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              # 时间统计
import gc                                # 垃圾回收模块
import os,sys                                # 操作系统接口模块
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
# 人脸mesh kmodel输入shape
fm_kmodel_input_shape = (1,3,120,120)
fmpost_kmodel_input_shapes = [(3,3),(3,1),(40,1),(10,1)]
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
# 人脸mesh kmodel
fm_kmodel_file = root_dir + 'kmodel/face_alignment.kmodel'
# 人脸mesh后处理kmodel
fmpost_kmodel_file = root_dir + 'kmodel/face_alignment_post.kmodel'
# anchor文件
anchors_path = root_dir + 'utils/prior_data_320.bin'
# 人脸mesh参数均值
param_mean = np.array([0.0003492636315058917,2.52790130161884e-07,-6.875197868794203e-07,60.1679573059082,-6.295513230725192e-07,0.0005757200415246189,-5.085391239845194e-05,74.2781982421875,5.400917189035681e-07,6.574138387804851e-05,0.0003442012530285865,-66.67157745361328,-346603.6875,-67468.234375,46822.265625,-15262.046875,4350.5888671875,-54261.453125,-18328.033203125,-1584.328857421875,-84566.34375,3835.960693359375,-20811.361328125,38094.9296875,-19967.85546875,-9241.3701171875,-19600.71484375,13168.08984375,-5259.14404296875,1848.6478271484375,-13030.662109375,-2435.55615234375,-2254.20654296875,-14396.5615234375,-6176.3291015625,-25621.919921875,226.39447021484375,-6326.12353515625,-10867.2509765625,868.465087890625,-5831.14794921875,2705.123779296875,-3629.417724609375,2043.9901123046875,-2446.6162109375,3658.697021484375,-7645.98974609375,-6674.45263671875,116.38838958740234,7185.59716796875,-1429.48681640625,2617.366455078125,-1.2070955038070679,0.6690792441368103,-0.17760828137397766,0.056725528091192245,0.03967815637588501,-0.13586315512657166,-0.09223993122577667,-0.1726071834564209,-0.015804484486579895,-0.1416848599910736],dtype=np.float)
# 人脸mesh参数方差
param_std = np.array([0.00017632152594160289,6.737943476764485e-05,0.00044708489440381527,26.55023193359375,0.0001231376954820007,4.493021697271615e-05,7.923670636955649e-05,6.982563018798828,0.0004350444069132209,0.00012314890045672655,0.00017400001524947584,20.80303955078125,575421.125,277649.0625,258336.84375,255163.125,150994.375,160086.109375,111277.3046875,97311.78125,117198.453125,89317.3671875,88493.5546875,72229.9296875,71080.2109375,50013.953125,55968.58203125,47525.50390625,49515.06640625,38161.48046875,44872.05859375,46273.23828125,38116.76953125,28191.162109375,32191.4375,36006.171875,32559.892578125,25551.1171875,24267.509765625,27521.3984375,23166.53125,21101.576171875,19412.32421875,19452.203125,17454.984375,22537.623046875,16174.28125,14671.640625,15115.6884765625,13870.0732421875,13746.3125,12663.1337890625,1.5870834589004517,1.5077009201049805,0.5881357789039612,0.5889744758605957,0.21327851712703705,0.2630201280117035,0.2796429395675659,0.38030216097831726,0.16162841022014618,0.2559692859649658],dtype=np.float)
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
# fm_ai2d：              人脸mesh ai2d实例
# fm_ai2d_input_tensor： 人脸mesh ai2d输入
# fm_ai2d_output_tensor：人脸mesh ai2d输入
# fm_ai2d_builder：      根据人脸mesh ai2d参数，构建的人脸mesh ai2d_builder对象
global fm_ai2d,fm_ai2d_input_tensor,fm_ai2d_output_tensor,fm_ai2d_builder
global roi               #人脸区域
global vertices          #3D关键点

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
        if 'fd_ai2d' in globals():         #删除人脸检测ai2d变量，释放对它所引用对象的内存引用
            global fd_ai2d
            del fd_ai2d
        if 'fd_ai2d_output_tensor' in globals():  #删除人脸检测ai2d_output_tensor变量，释放对它所引用对象的内存引用
            global fd_ai2d_output_tensor
            del fd_ai2d_output_tensor

###############for face recognition###############
def parse_roi_box_from_bbox(bbox):
    # 获取人脸roi
    x1, y1, w, h = map(lambda x: int(round(x, 0)), bbox[:4])
    old_size = (w + h) / 2
    center_x = x1 + w / 2
    center_y = y1 + h / 2 + old_size * 0.14
    size = int(old_size * 1.58)

    x0 = center_x - float(size) / 2
    y0 = center_y - float(size) / 2
    x1 = x0 + size
    y1 = y0 + size

    x0 = max(0, min(x0, OUT_RGB888P_WIDTH))
    y0 = max(0, min(y0, OUT_RGB888P_HEIGH))
    x1 = max(0, min(x1, OUT_RGB888P_WIDTH))
    y1 = max(0, min(y1, OUT_RGB888P_HEIGH))

    roi = (x0, y0, x1 - x0, y1 - y0)
    return roi

def fm_ai2d_init():
    # 人脸mesh ai2d初始化
    with ScopedTiming("fm_ai2d_init",debug_mode > 0):
        # （1）创建人脸mesh ai2d对象
        global fm_ai2d
        fm_ai2d = nn.ai2d()

        # （2）创建人脸mesh ai2d_output_tensor对象，用于存放ai2d输出
        global fm_ai2d_output_tensor
        data = np.ones(fm_kmodel_input_shape, dtype=np.uint8)
        fm_ai2d_output_tensor = nn.from_numpy(data)

def fm_ai2d_run(rgb888p_img,det):
    # 人脸mesh ai2d推理
    with ScopedTiming("fm_ai2d_run",debug_mode > 0):
        global fm_ai2d,fm_ai2d_input_tensor,fm_ai2d_output_tensor
        #（1）根据原图ai2d_input_tensor对象
        ai2d_input = rgb888p_img.to_numpy_ref()
        fm_ai2d_input_tensor = nn.from_numpy(ai2d_input)

        # （2）根据新的det设置新的人脸mesh ai2d参数
        fm_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        global roi
        roi = parse_roi_box_from_bbox(det)
        fm_ai2d.set_crop_param(True,int(roi[0]),int(roi[1]),int(roi[2]),int(roi[3]))
        fm_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
        # （3）根据新的人脸mesh ai2d参数，构建人脸mesh ai2d_builder
        global fm_ai2d_builder
        fm_ai2d_builder = fm_ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], fm_kmodel_input_shape)
        # （4）推理人脸mesh ai2d，将预处理的结果保存到fm_ai2d_output_tensor
        fm_ai2d_builder.run(fm_ai2d_input_tensor, fm_ai2d_output_tensor)

def fm_ai2d_release():
    # 释放人脸mesh ai2d_input_tensor、ai2d_builder
    with ScopedTiming("fm_ai2d_release",debug_mode > 0):
        global fm_ai2d_input_tensor,fm_ai2d_builder
        del fm_ai2d_input_tensor
        del fm_ai2d_builder

def fm_kpu_init(kmodel_file):
    # 人脸mesh kpu初始化
    with ScopedTiming("fm_kpu_init",debug_mode > 0):
        # 初始化人脸mesh kpu对象
        kpu_obj = nn.kpu()
        # 加载人脸mesh kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化人脸mesh ai2d
        fm_ai2d_init()
        return kpu_obj

def fm_kpu_pre_process(rgb888p_img,det):
    # 人脸mesh kpu预处理
    # 人脸mesh ai2d推理，根据det对原图进行预处理
    fm_ai2d_run(rgb888p_img,det)
    with ScopedTiming("fm_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fm_ai2d_output_tensor
        # 将人脸mesh ai2d输出设置为人脸mesh kpu输入
        current_kmodel_obj.set_input_tensor(0, fm_ai2d_output_tensor)

def fm_kpu_get_output():
    with ScopedTiming("fm_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取人脸mesh kpu输出
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

def fm_kpu_post_process(param):
    # 人脸mesh kpu结果后处理，反标准化
    with ScopedTiming("fm_kpu_post_process",debug_mode > 0):
        param = param * param_std + param_mean
    return param

def fm_kpu_run(kpu_obj,rgb888p_img,det):
    # 人脸mesh kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）人脸mesh kpu预处理，设置kpu输入
    fm_kpu_pre_process(rgb888p_img,det)
    # （2）人脸mesh kpu推理
    with ScopedTiming("fm_kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放人脸mesh ai2d
    fm_ai2d_release()
    # （4）获取人脸mesh kpu输出
    param = fm_kpu_get_output()
    # （5）人脸mesh 后处理
    param = fm_kpu_post_process(param)
    return param

def fm_kpu_deinit():
    # 人脸mesh kpu释放
    with ScopedTiming("fm_kpu_deinit",debug_mode > 0):
        if 'fm_ai2d' in globals():         # 删除fm_ai2d变量，释放对它所引用对象的内存引用
            global fm_ai2d
            del fm_ai2d
        if 'fm_ai2d_output_tensor' in globals():  # 删除fm_ai2d_output_tensor变量，释放对它所引用对象的内存引用
            global fm_ai2d_output_tensor
            del fm_ai2d_output_tensor

def fmpost_kpu_init(kmodel_file):
    # face mesh post模型初始化
    with ScopedTiming("fmpost_kpu_init",debug_mode > 0):
        # 初始化人脸mesh kpu post对象
        kpu_obj = nn.kpu()
        # 加载人脸mesh后处理kmodel
        kpu_obj.load_kmodel(kmodel_file)
        return kpu_obj

def fmpost_kpu_pre_process(param):
    # face mesh post模型预处理，param解析
    with ScopedTiming("fmpost_kpu_pre_process",debug_mode > 0):
        param = param[0]
        trans_dim, shape_dim, exp_dim = 12, 40, 10

        # reshape前务必进行copy，否则会导致模型输入错误
        R_ = param[:trans_dim].copy().reshape((3, -1))
        R = R_[:, :3].copy()
        offset = R_[:, 3].copy()
        offset = offset.reshape((3, 1))
        alpha_shp = param[trans_dim:trans_dim + shape_dim].copy().reshape((-1, 1))
        alpha_exp = param[trans_dim + shape_dim:].copy().reshape((-1, 1))

        R_tensor = nn.from_numpy(R)
        current_kmodel_obj.set_input_tensor(0, R_tensor)
        del R_tensor

        offset_tensor = nn.from_numpy(offset)
        current_kmodel_obj.set_input_tensor(1, offset_tensor)
        del offset_tensor

        alpha_shp_tensor = nn.from_numpy(alpha_shp)
        current_kmodel_obj.set_input_tensor(2, alpha_shp_tensor)
        del alpha_shp_tensor

        alpha_exp_tensor = nn.from_numpy(alpha_exp)
        current_kmodel_obj.set_input_tensor(3, alpha_exp_tensor)
        del alpha_exp_tensor

    return

def fmpost_kpu_get_output():
    # 获取face mesh post模型输出
    with ScopedTiming("fmpost_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        # 获取人脸mesh kpu输出
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result

def fmpost_kpu_post_process(roi):
    # face mesh post模型推理结果后处理
    with ScopedTiming("fmpost_kpu_post_process",debug_mode > 0):
        x, y, w, h = map(lambda x: int(round(x, 0)), roi[:4])
        x = x * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
        y = y * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
        w = w * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
        h = h * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
        roi_array = np.array([x,y,w,h],dtype=np.float)
        global vertices
        aidemo.face_mesh_post_process(roi_array,vertices)
    return

def fmpost_kpu_run(kpu_obj,param):
    # face mesh post模型推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    fmpost_kpu_pre_process(param)
    with ScopedTiming("fmpost_kpu_run",debug_mode > 0):
        kpu_obj.run()
    global vertices
    vertices = fmpost_kpu_get_output()
    global roi
    fmpost_kpu_post_process(roi)
    return
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

def display_draw(dets,vertices_list):
    # 在显示器画人脸轮廓
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img_ulab,draw_img,osd_img
        if dets:
            draw_img.clear()
            for vertices in vertices_list:
                aidemo.face_draw_mesh(draw_img_ulab, vertices)
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
def face_mesh_inference():
    # 人脸检测kpu初始化
    kpu_face_detect = fd_kpu_init(fd_kmodel_file)
    # 人脸mesh kpu初始化
    kpu_face_mesh = fm_kpu_init(fm_kmodel_file)
    # face_mesh_post kpu初始化
    kpu_face_mesh_post = fmpost_kpu_init(fmpost_kmodel_file)
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
                    ## （2.2）针对每个人脸框，推理得到对应人脸mesh
                    mesh_result = []
                    for det in dets:
                        param = fm_kpu_run(kpu_face_mesh,rgb888p_img,det)
                        fmpost_kpu_run(kpu_face_mesh_post,param)
                        global vertices
                        mesh_result.append(vertices)
                    ## （2.3）将人脸mesh 画到屏幕上
                    display_draw(dets,mesh_result)

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
        fd_kpu_deinit()
        fm_kpu_deinit()
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_face_detect
        del kpu_face_mesh
        del kpu_face_mesh_post
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

    print("face_mesh_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    face_mesh_inference()
