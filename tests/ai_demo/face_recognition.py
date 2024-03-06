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

# kmodel输入shape
# 人脸检测kmodel输入shape
fd_kmodel_input_shape = (1,3,320,320)
# 人脸识别kmodel输入shape
fr_kmodel_input_shape = (1,3,112,112)
# ai原图padding
rgb_mean = [104,117,123]

#kmodel相关参数设置
#人脸检测
confidence_threshold = 0.5               #人脸检测阈值
top_k = 5000
nms_threshold = 0.2
keep_top_k = 750
vis_thres = 0.5
variance = [0.1, 0.2]
anchor_len = 4200
score_dim = 2
det_dim = 4
keypoint_dim = 10
#人脸识别
max_register_face = 100                  # 数据库最多人脸个数
feature_num = 128                        # 人脸识别特征维度
face_recognition_threshold = 0.75        # 人脸识别阈值

#文件配置
# 人脸检测kmodel
root_dir = '/sdcard/app/tests/'
fd_kmodel_file = root_dir + 'kmodel/face_detection_320.kmodel'
# 人脸识别kmodel
fr_kmodel_file = root_dir + 'kmodel/face_recognition.kmodel'
# 人脸检测anchor
anchors_path = root_dir + 'utils/prior_data_320.bin'
# 人脸数据库
database_dir = root_dir + 'utils/db/'
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
global current_kmodel_obj #当前kpu实例
# fd_ai2d：               人脸检测ai2d实例
# fd_ai2d_input_tensor：  人脸检测ai2d输入
# fd_ai2d_output_tensor： 人脸检测ai2d输入
# fd_ai2d_builder：       根据人脸检测ai2d参数，构建的人脸检测ai2d_builder对象
global fd_ai2d,fd_ai2d_input_tensor,fd_ai2d_output_tensor,fd_ai2d_builder
# fr_ai2d：               人脸识别ai2d实例
# fr_ai2d_input_tensor：  人脸识别ai2d输入
# fr_ai2d_output_tensor： 人脸识别ai2d输入
# fr_ai2d_builder：       根据人脸识别ai2d参数，构建的人脸识别ai2d_builder对象
global fr_ai2d,fr_ai2d_input_tensor,fr_ai2d_output_tensor,fr_ai2d_builder
# valid_register_face：   数据库中有效人脸个数
# db_name：               数据库人名列表
# db_data：               数据库特征列表
global valid_register_face,db_name，db_data

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
        return post_ret,post_ret
    else:
        return post_ret[0],post_ret[1]          #0:det,1:landm,2:score

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
##for database
def database_init():
    # 数据初始化，构建数据库人名列表和数据库特征列表
    with ScopedTiming("database_init", debug_mode > 1):
        global valid_register_face,db_name,db_data
        valid_register_face = 0
        db_name = []
        db_data = []

        db_file_list = os.listdir(database_dir)
        for db_file in db_file_list:
            if not db_file.endswith('.bin'):
                continue
            if valid_register_face >= max_register_face:
                break
            valid_index = valid_register_face
            full_db_file = database_dir + db_file
            with open(full_db_file, 'rb') as f:
                data = f.read()
            feature = np.frombuffer(data, dtype=np.float)
            db_data.append(feature)
            name = db_file.split('.')[0]
            db_name.append(name)
            valid_register_face += 1

def database_reset():
    # 数据库清空
    with ScopedTiming("database_reset", debug_mode > 1):
        global valid_register_face,db_name,db_data
        print("database clearing...")
        db_name = []
        db_data = []
        valid_register_face = 0
        print("database clear Done!")

def database_search(feature):
    # 数据库查询
    with ScopedTiming("database_search", debug_mode > 1):
        global valid_register_face,db_name,db_data
        v_id = -1
        v_score_max = 0.0

        # 将当前人脸特征归一化
        feature /= np.linalg.norm(feature)
        # 遍历当前人脸数据库，统计最高得分
        for i in range(valid_register_face):
            db_feature = db_data[i]
            db_feature /= np.linalg.norm(db_feature)
            # 计算数据库特征与当前人脸特征相似度
            v_score = np.dot(feature, db_feature)/2 + 0.5
            if v_score > v_score_max:
                v_score_max = v_score
                v_id = i

        if v_id == -1:
            # 数据库中无人脸
            return 'unknown'
        elif v_score_max < face_recognition_threshold:
            # 小于人脸识别阈值，未识别
#            print('v_score_max:',v_score_max)
            return 'unknown'
        else:
            # 识别成功
            result = 'name: {}, score:{}'.format(db_name[v_id],v_score_max)
            return result

# 标准5官
umeyama_args_112 = [
    38.2946 , 51.6963 ,
    73.5318 , 51.5014 ,
    56.0252 , 71.7366 ,
    41.5493 , 92.3655 ,
    70.7299 , 92.2041
]

def svd22(a):
    # svd
    s = [0.0, 0.0]
    u = [0.0, 0.0, 0.0, 0.0]
    v = [0.0, 0.0, 0.0, 0.0]

    s[0] = (math.sqrt((a[0] - a[3]) ** 2 + (a[1] + a[2]) ** 2) + math.sqrt((a[0] + a[3]) ** 2 + (a[1] - a[2]) ** 2)) / 2
    s[1] = abs(s[0] - math.sqrt((a[0] - a[3]) ** 2 + (a[1] + a[2]) ** 2))
    v[2] = math.sin((math.atan2(2 * (a[0] * a[1] + a[2] * a[3]), a[0] ** 2 - a[1] ** 2 + a[2] ** 2 - a[3] ** 2)) / 2) if \
    s[0] > s[1] else 0
    v[0] = math.sqrt(1 - v[2] ** 2)
    v[1] = -v[2]
    v[3] = v[0]
    u[0] = -(a[0] * v[0] + a[1] * v[2]) / s[0] if s[0] != 0 else 1
    u[2] = -(a[2] * v[0] + a[3] * v[2]) / s[0] if s[0] != 0 else 0
    u[1] = (a[0] * v[1] + a[1] * v[3]) / s[1] if s[1] != 0 else -u[2]
    u[3] = (a[2] * v[1] + a[3] * v[3]) / s[1] if s[1] != 0 else u[0]
    v[0] = -v[0]
    v[2] = -v[2]

    return u, s, v


def image_umeyama_112(src):
    # 使用Umeyama算法计算仿射变换矩阵
    SRC_NUM = 5
    SRC_DIM = 2
    src_mean = [0.0, 0.0]
    dst_mean = [0.0, 0.0]

    for i in range(0,SRC_NUM * 2,2):
        src_mean[0] += src[i]
        src_mean[1] += src[i + 1]
        dst_mean[0] += umeyama_args_112[i]
        dst_mean[1] += umeyama_args_112[i + 1]

    src_mean[0] /= SRC_NUM
    src_mean[1] /= SRC_NUM
    dst_mean[0] /= SRC_NUM
    dst_mean[1] /= SRC_NUM

    src_demean = [[0.0, 0.0] for _ in range(SRC_NUM)]
    dst_demean = [[0.0, 0.0] for _ in range(SRC_NUM)]

    for i in range(SRC_NUM):
        src_demean[i][0] = src[2 * i] - src_mean[0]
        src_demean[i][1] = src[2 * i + 1] - src_mean[1]
        dst_demean[i][0] = umeyama_args_112[2 * i] - dst_mean[0]
        dst_demean[i][1] = umeyama_args_112[2 * i + 1] - dst_mean[1]

    A = [[0.0, 0.0], [0.0, 0.0]]
    for i in range(SRC_DIM):
        for k in range(SRC_DIM):
            for j in range(SRC_NUM):
                A[i][k] += dst_demean[j][i] * src_demean[j][k]
            A[i][k] /= SRC_NUM

    T = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    U, S, V = svd22([A[0][0], A[0][1], A[1][0], A[1][1]])

    T[0][0] = U[0] * V[0] + U[1] * V[2]
    T[0][1] = U[0] * V[1] + U[1] * V[3]
    T[1][0] = U[2] * V[0] + U[3] * V[2]
    T[1][1] = U[2] * V[1] + U[3] * V[3]

    scale = 1.0
    src_demean_mean = [0.0, 0.0]
    src_demean_var = [0.0, 0.0]
    for i in range(SRC_NUM):
        src_demean_mean[0] += src_demean[i][0]
        src_demean_mean[1] += src_demean[i][1]

    src_demean_mean[0] /= SRC_NUM
    src_demean_mean[1] /= SRC_NUM

    for i in range(SRC_NUM):
        src_demean_var[0] += (src_demean_mean[0] - src_demean[i][0]) * (src_demean_mean[0] - src_demean[i][0])
        src_demean_var[1] += (src_demean_mean[1] - src_demean[i][1]) * (src_demean_mean[1] - src_demean[i][1])

    src_demean_var[0] /= SRC_NUM
    src_demean_var[1] /= SRC_NUM

    scale = 1.0 / (src_demean_var[0] + src_demean_var[1]) * (S[0] + S[1])
    T[0][2] = dst_mean[0] - scale * (T[0][0] * src_mean[0] + T[0][1] * src_mean[1])
    T[1][2] = dst_mean[1] - scale * (T[1][0] * src_mean[0] + T[1][1] * src_mean[1])
    T[0][0] *= scale
    T[0][1] *= scale
    T[1][0] *= scale
    T[1][1] *= scale
    return T

def get_affine_matrix(sparse_points):
    # 获取放射变换矩阵
    with ScopedTiming("get_affine_matrix", debug_mode > 1):
        # 使用Umeyama算法计算仿射变换矩阵
        matrix_dst = image_umeyama_112(sparse_points)
        matrix_dst = [matrix_dst[0][0],matrix_dst[0][1],matrix_dst[0][2],
            matrix_dst[1][0],matrix_dst[1][1],matrix_dst[1][2]]
        return matrix_dst

def fr_ai2d_init():
    with ScopedTiming("fr_ai2d_init",debug_mode > 0):
        # （1）人脸识别ai2d初始化
        global fr_ai2d
        fr_ai2d = nn.ai2d()

        # （2）人脸识别ai2d_output_tensor初始化，用于存放ai2d输出
        global fr_ai2d_output_tensor
        data = np.ones(fr_kmodel_input_shape, dtype=np.uint8)
        fr_ai2d_output_tensor = nn.from_numpy(data)

def fr_ai2d_run(rgb888p_img,sparse_points):
    # 人脸识别ai2d推理
    with ScopedTiming("fr_ai2d_run",debug_mode > 0):
        global fr_ai2d,fr_ai2d_input_tensor,fr_ai2d_output_tensor
        #（1）根据原图创建人脸识别ai2d_input_tensor对象
        ai2d_input = rgb888p_img.to_numpy_ref()
        fr_ai2d_input_tensor = nn.from_numpy(ai2d_input)
        #（2）根据新的人脸关键点设置新的人脸识别ai2d参数
        fr_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        affine_matrix = get_affine_matrix(sparse_points)
        fr_ai2d.set_affine_param(True,nn.interp_method.cv2_bilinear,0, 0, 127, 1,affine_matrix)
        global fr_ai2d_builder
        # （3）根据新的人脸识别ai2d参数，构建识别ai2d_builder
        fr_ai2d_builder = fr_ai2d.build([1,3,OUT_RGB888P_HEIGH,OUT_RGB888P_WIDTH], fr_kmodel_input_shape)
        # （4）推理人脸识别ai2d，将预处理的结果保存到fr_ai2d_output_tensor
        fr_ai2d_builder.run(fr_ai2d_input_tensor, fr_ai2d_output_tensor)

def fr_ai2d_release():
    # 释放人脸识别ai2d_input_tensor、ai2d_builder
    with ScopedTiming("fr_ai2d_release",debug_mode > 0):
        global fr_ai2d_input_tensor,fr_ai2d_builder
        del fr_ai2d_input_tensor
        del fr_ai2d_builder

def fr_kpu_init(kmodel_file):
    # 人脸识别kpu初始化
    with ScopedTiming("fr_kpu_init",debug_mode > 0):
        # 初始化人脸识别kpu对象
        kpu_obj = nn.kpu()
        # 加载人脸识别kmodel
        kpu_obj.load_kmodel(kmodel_file)
        # 初始化人脸识别ai2d
        fr_ai2d_init()
        # 数据库初始化
        database_init()
        return kpu_obj

def fr_kpu_pre_process(rgb888p_img,sparse_points):
    # 人脸识别kpu预处理
    # 人脸识别ai2d推理，根据关键点对原图进行预处理
    fr_ai2d_run(rgb888p_img,sparse_points)
    with ScopedTiming("fr_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fr_ai2d_output_tensor
        # 将人脸识别ai2d输出设置为人脸识别kpu输入
        current_kmodel_obj.set_input_tensor(0, fr_ai2d_output_tensor)

def fr_kpu_get_output():
    # 获取人脸识别kpu输出
    with ScopedTiming("fr_kpu_get_output",debug_mode > 0):
        global current_kmodel_obj
        data = current_kmodel_obj.get_output_tensor(0)
        result = data.to_numpy()
        del data
        return result[0]

def fr_kpu_run(kpu_obj,rgb888p_img,sparse_points):
    # 人脸识别kpu推理
    global current_kmodel_obj
    current_kmodel_obj = kpu_obj
    # （1）人脸识别kpu预处理,设置kpu输入
    fr_kpu_pre_process(rgb888p_img,sparse_points)
    # （2）人脸识别kpu推理
    with ScopedTiming("fr kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）释放人脸识别ai2d
    fr_ai2d_release()
    # （4）获取人脸识别kpu输出
    results = fr_kpu_get_output()
    # （5）在数据库中查找当前人脸特征
    recg_result = database_search(results)
    return recg_result

def fr_kpu_deinit():
    # 人脸识别kpu相关资源释放
    with ScopedTiming("fr_kpu_deinit",debug_mode > 0):
        if 'fr_ai2d' in globals():
            global fr_ai2d
            del fr_ai2d
        if 'fr_ai2d_output_tensor' in globals():
            global fr_ai2d_output_tensor
            del fr_ai2d_output_tensor

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

def display_draw(dets,recg_results):
    # 在显示器上写人脸识别结果
    with ScopedTiming("display_draw",debug_mode >0):
        global draw_img,osd_img
        if dets:
            draw_img.clear()
            for i,det in enumerate(dets):
                # （1）画人脸框
                x1, y1, w, h = map(lambda x: int(round(x, 0)), det[:4])
                x1 = x1 * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                y1 = y1 * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
                w =  w * DISPLAY_WIDTH // OUT_RGB888P_WIDTH
                h = h * DISPLAY_HEIGHT // OUT_RGB888P_HEIGH
                draw_img.draw_rectangle(x1,y1, w, h, color=(255,0, 0, 255), thickness = 4)

                # （2）写人脸识别结果
                recg_text = recg_results[i]
                draw_img.draw_string(x1,y1,recg_text,color=(255, 255, 0, 0),scale=4)

            # （3）将画图结果拷贝到osd
            draw_img.copy_to(osd_img)
            # （4）将osd显示到屏幕
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

    global buffer, draw_img, osd_img
    buffer = media.request_buffer(4 * DISPLAY_WIDTH * DISPLAY_HEIGHT)
    # 用于画框
    draw_img = image.Image(DISPLAY_WIDTH, DISPLAY_HEIGHT, image.ARGB8888)
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
def face_recognition_inference():
    print("face_recognition_test start")
    # 人脸检测kpu初始化
    kpu_face_detect = fd_kpu_init(fd_kmodel_file)
    # 人脸关键点kpu初始化
    kpu_face_recg = fr_kpu_init(fr_kmodel_file)
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
                    dets,landms = fd_kpu_run(kpu_face_detect,rgb888p_img)
                    recg_result = []
                    for landm in landms:
                        # （2.2）针对每个人脸五官点，推理得到人脸特征，并计算特征在数据库中相似度
                        ret = fr_kpu_run(kpu_face_recg,rgb888p_img,landm)
                        recg_result.append(ret)
                    # （2.3）将识别结果画到显示器上
                    display_draw(dets,recg_result)

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
        fr_kpu_deinit()
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_face_detect
        del kpu_face_recg
        # 垃圾回收
        gc.collect()
        nn.shrink_memory_pool()
        # 释放媒体资源
        media_deinit()

    print("face_recognition_test end")
    return 0

if __name__ == '__main__':
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    face_recognition_inference()
