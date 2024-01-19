import ulab.numpy as np                  #类似python numpy操作，但也会有一些接口不同
import nncase_runtime as nn              #nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
import aidemo                            #aidemo模块，封装ai demo相关后处理、画图操作
import image                             #图像模块，主要用于读取、图像绘制元素（框、点等）等操作
import time                              #时间统计
import gc                                #垃圾回收模块
import os,sys                                #操作系统接口模块
import math                              #数学模块

#********************for config.py********************
# kmodel输入shape
# 人脸检测kmodel输入shape
fd_kmodel_input_shape = (1,3,320,320)
# 人脸识别kmodel输入shape
fr_kmodel_input_shape = (1,3,112,112)
# ai原图padding
rgb_mean = [104,117,123]

#kmodel相关参数设置
#人脸检测
confidence_threshold = 0.5           #人脸检测阈值
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
max_register_face = 100              #数据库最多人脸个数
feature_num = 128                    #人脸识别特征维度

# 文件配置
# 人脸检测kmodel
root_dir = '/sdcard/app/tests/'
fd_kmodel_file = root_dir + 'kmodel/face_detection_320.kmodel'
# 人脸识别kmodel
fr_kmodel_file = root_dir + 'kmodel/face_recognition.kmodel'
# 人脸检测anchor
anchors_path = root_dir + 'utils/prior_data_320.bin'
# 人脸注册数据库
database_dir = root_dir + 'utils/db/'
# 人脸注册数据库原图
database_img_dir = root_dir + 'utils/db_img/'
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
global valid_register_face #数据库中有效人脸个数

#读取anchor文件，为人脸检测后处理做准备
print('anchors_path:',anchors_path)
prior_data = np.fromfile(anchors_path, dtype=np.float)
prior_data = prior_data.reshape((anchor_len,det_dim))

def get_pad_one_side_param(rgb888p_img):
    # 右padding或下padding，获取padding参数
    with ScopedTiming("get_pad_one_side_param", debug_mode > 1):
        dst_w = fd_kmodel_input_shape[3]                         # kmodel输入宽（w）
        dst_h = fd_kmodel_input_shape[2]                         # kmodel输入高（h）

        # 计算最小的缩放比例，等比例缩放
        ratio_w = dst_w / rgb888p_img.shape[3]
        ratio_h = dst_h / rgb888p_img.shape[2]
        if ratio_w < ratio_h:
            ratio = ratio_w
        else:
            ratio = ratio_h

        # 计算经过缩放后的新宽和新高
        new_w = (int)(ratio * rgb888p_img.shape[3])
        new_h = (int)(ratio * rgb888p_img.shape[2])

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

        #（2）创建人脸检测ai2d_output_tensor，用于保存人脸检测ai2d输出
        global fd_ai2d_output_tensor
        data = np.ones(fd_kmodel_input_shape, dtype=np.uint8)
        fd_ai2d_output_tensor = nn.from_numpy(data)

def fd_ai2d_run(rgb888p_img):
    # 根据人脸检测ai2d参数，对原图rgb888p_img进行预处理
    with ScopedTiming("fd_ai2d_run",debug_mode > 0):
        global fd_ai2d,fd_ai2d_input_tensor,fd_ai2d_output_tensor,fd_ai2d_builder
        # （1）根据原图构建ai2d_input_tensor对象
        fd_ai2d_input_tensor = nn.from_numpy(rgb888p_img)
        # （2）根据新的图像设置新的人脸检测ai2d参数
        fd_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        fd_ai2d.set_pad_param(True, get_pad_one_side_param(rgb888p_img), 0, rgb_mean)
        fd_ai2d.set_resize_param(True, nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
        # （3）根据新的人脸检测ai2d参数，构建人脸检测ai2d_builder
        fd_ai2d_builder = fd_ai2d.build(rgb888p_img.shape, fd_kmodel_input_shape)
        # （4）运行人脸检测ai2d_builder，将结果保存到人脸检测ai2d_output_tensor中
        fd_ai2d_builder.run(fd_ai2d_input_tensor, fd_ai2d_output_tensor)

def fd_ai2d_release():
    # 释放人脸检测ai2d部分资源
    with ScopedTiming("fd_ai2d_release",debug_mode > 0):
        global fd_ai2d_input_tensor,fd_ai2d_builder
        del fd_ai2d_input_tensor
        del fd_ai2d_builder


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
                [rgb888p_img.shape[3],rgb888p_img.shape[2]],results)
    # （6）返回人脸关键点
    if len(post_ret)==0:
        return post_ret
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
    # 获取affine变换矩阵
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
        fr_ai2d_input_tensor = nn.from_numpy(rgb888p_img)
        #（2）根据新的人脸关键点设置新的人脸识别ai2d参数
        fr_ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,
                                       nn.ai2d_format.NCHW_FMT,
                                       np.uint8, np.uint8)
        affine_matrix = get_affine_matrix(sparse_points)
        fr_ai2d.set_affine_param(True,nn.interp_method.cv2_bilinear,0, 0, 127, 1,affine_matrix)
        global fr_ai2d_builder
        # （3）根据新的人脸识别ai2d参数，构建识别ai2d_builder
        fr_ai2d_builder = fr_ai2d.build(rgb888p_img.shape, fr_kmodel_input_shape)
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
        return kpu_obj

def fr_kpu_pre_process(rgb888p_img,sparse_points):
    # 人脸识别kpu预处理
    # 人脸识别ai2d推理，根据关键点对原图进行预处理
    fr_ai2d_run(rgb888p_img,sparse_points)
    with ScopedTiming("fr_kpu_pre_process",debug_mode > 0):
        global current_kmodel_obj,fr_ai2d_output_tensor
        # 将人脸识别ai2d输出设置为人脸识别kpu输入
        current_kmodel_obj.set_input_tensor(0, fr_ai2d_output_tensor)

        #ai2d_out_data = fr_ai2d_output_tensor.to_numpy()
        #print('ai2d_out_data.shape:',ai2d_out_data.shape)
        #with open("/sdcard/app/ai2d_out.bin", "wb") as file:
            #file.write(ai2d_out_data.tobytes())

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
    return results

def fr_kpu_deinit():
    # 人脸识别kpu相关资源释放
    with ScopedTiming("fr_kpu_deinit",debug_mode > 0):
        if 'fr_ai2d' in globals():
            global fr_ai2d
            del fr_ai2d
        if 'fr_ai2d_output_tensor' in globals():
            global fr_ai2d_output_tensor
            del fr_ai2d_output_tensor

#********************for face_detect.py********************
def image2rgb888array(img):   #4维
    # 将Image转换为rgb888格式
    with ScopedTiming("fr_kpu_deinit",debug_mode > 0):
        img_data_rgb888=img.to_rgb888()
        # hwc,rgb888
        img_hwc=img_data_rgb888.to_numpy_ref()
        shape=img_hwc.shape
        img_tmp = img_hwc.reshape((shape[0] * shape[1], shape[2]))
        img_tmp_trans = img_tmp.transpose()
        img_res=img_tmp_trans.copy()
        # chw,rgb888
        img_return=img_res.reshape((1,shape[2],shape[0],shape[1]))
    return  img_return

def face_registration_inference():
    print("face_registration_test start")
    # 人脸检测kpu初始化
    kpu_face_detect = fd_kpu_init(fd_kmodel_file)
    # 人脸识别kpu初始化
    kpu_face_reg = fr_kpu_init(fr_kmodel_file)
    try:
        # 获取图像列表
        img_list = os.listdir(database_img_dir)
        for img_file in img_list:
            os.exitpoint()
            with ScopedTiming("total",1):
                # （1）读取一张图像
                full_img_file = database_img_dir + img_file
                print(full_img_file)
                img = image.Image(full_img_file)
                rgb888p_img_ndarry = image2rgb888array(img)

                #（2）推理得到人脸检测kpu，得到人脸检测框、人脸五点
                dets,landms = fd_kpu_run(kpu_face_detect,rgb888p_img_ndarry)
                if dets:
                    if dets.shape[0] == 1:
                        #（3）若是只检测到一张人脸，则将该人脸注册到数据库
                        db_i_name = img_file.split('.')[0]
                        for landm in landms:
                            reg_result = fr_kpu_run(kpu_face_reg,rgb888p_img_ndarry,landm)
                            #print('\nwrite bin:',database_dir+'{}.bin'.format(db_i_name))
                            with open(database_dir+'{}.bin'.format(db_i_name), "wb") as file:
                                file.write(reg_result.tobytes())
                    else:
                        print('Only one person in a picture when you sign up')
                else:
                    print('No person detected')

                gc.collect()
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        # 释放kpu资源
        fd_kpu_deinit()
        fr_kpu_deinit()
        if 'current_kmodel_obj' in globals():
            global current_kmodel_obj
            del current_kmodel_obj
        del kpu_face_detect
        del kpu_face_reg
        # 垃圾回收
        gc.collect()
        nn.shrink_memory_pool()

    print("face_registration_test end")
    return 0

if __name__ == '__main__':
    nn.shrink_memory_pool()
    face_registration_inference()
