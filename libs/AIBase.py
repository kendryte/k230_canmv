import os
import ujson
from media import _sensor
from media.lcd import *
from media.media import *
from time import *
import nncase_runtime as nn
import ulab.numpy as np
import time
import utime
import image
import random
import gc
import sys

# AIBase类别主要抽象的是AI任务推理流程
class AIBase:
    def __init__(self,kmodel_path,input_size,rgb888p_size,debug_mode=0):
        # kmodel路径
        self.kmodel_path=kmodel_path
        # 模型输入分辨率
        self.input_size=input_size
        # sensor给到AI的图像分辨率
        self.rgb888p_size=rgb888p_size
        # 调试模式
        self.debug_mode=debug_mode
        
        # 预处理ai2d
        self.ai2d=nn.ai2d()
        # ai2d计算过程中的输入输出数据类型，输入输出数据格式
        self.ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)
        # ai2d构造器
        self.ai2d_builder=None
        # ai2d输入tensor对象
        self.ai2d_input_tensor=None
        # ai2d输出tensor对象
        self.ai2d_output_tensor=None
        # kpu对象
        self.kpu=nn.kpu()
        # 推理结果列表
        self.results=[]

    def init(self):
        # ai2d构造函数
        self.ai2d_builder = self.ai2d.build([1,3,self.rgb888p_size[1],self.rgb888p_size[0]], [1,3,self.input_size[1],self.input_size[0]])
        # 定义ai2d输出数据(即kmodel的输入数据，所以数据分辨率和模型的input_size一致)，并转换成tensor
        output_data = np.ones((1,3,self.input_size[1],self.input_size[0]),dtype=np.uint8)
        self.ai2d_output_tensor = nn.from_numpy(output_data)
        # 加载kmodel
        self.kpu.load_kmodel(self.kmodel_path)
    
    # 预处理crop函数
    # start_x：宽度方向的起始像素,int类型
    # start_y: 高度方向的起始像素,int类型
    # width: 宽度方向的crop长度,int类型
    # height: 高度方向的crop长度,int类型
    def init_crop(self,start_x,start_y,width,height):
        self.ai2d.set_crop_param(True,start_x,start_y,width,height)
        
    # 预处理shift函数
    # shift_val:右移的比特数,int类型
    def init_shift(self,shift_val):
        self.ai2d.set_shift_param(True,shift_val)
        
    # 预处理pad函数
    # paddings:各个维度的padding, size=8，分别表示dim0到dim4的前后padding的个数，其中dim0/dim1固定配置{0, 0},list类型
    # pad_mode:只支持pad constant，配置0即可,int类型
    # pad_val:每个channel的padding value,list类型
    def init_pad(self,paddings,pad_mode,pad_val):
        self.ai2d.set_pad_param(True,paddings,pad_mode,pad_val)
        
    # 预处理resize函数
    # interp_method:resize插值方法，ai2d_interp_method类型
    # interp_mode:resize模式，ai2d_interp_mode类型
    def init_resize(self,interp_method,interp_mode):
        self.ai2d.set_resize_param(True,interp_method,interp_mode)
        
    # 预处理affine函数
    # interp_method:Affine采用的插值方法,ai2d_interp_method类型
    # cord_round:整数边界0或者1,uint32_t类型
    # bound_ind:边界像素模式0或者1,uint32_t类型
    # bound_val:边界填充值,uint32_t类型
    # bound_smooth:边界平滑0或者1,uint32_t类型
    # M:仿射变换矩阵对应的vector，仿射变换为Y=[a_0, a_1; a_2, a_3] \cdot  X + [b_0, b_1] $, 则  M=[a_0,a_1,b_0,a_2,a_3,b_1 ],list类型
    def init_affine(self,interp_method,crop_round,bound_ind,bound_val,bound_smooth,M):
        self.ai2d.set_affine_param(True,interp_method,crop_round,bound_ind,bound_val,bound_smooth,M)
    
    # AI推理流程
    def run(self,input_np):
        # 将传入的一帧数据转换成tensor
        input_tensor = nn.from_numpy(input_np)
        # 运行ai2d做初始化
        self.ai2d_builder.run(input_tensor, self.ai2d_output_tensor)
        self.results.clear()
        # 将ai2d的输出tensor绑定为kmodel的输入数据
        self.kpu.set_input_tensor(0, self.ai2d_output_tensor)
        # 运行kmodel做推理
        self.kpu.run()
        # 获取kmodel的推理输出tensor,输出可能为多个，因此返回的是一个列表
        for i in range(self.kpu.outputs_size()):
            output_data = self.kpu.get_output_tensor(i)
            result = output_data.to_numpy()
            self.results.append(result)
            del output_data
        return self.results
    
    # BaseClassifier销毁函数
    def deinit(self):
        del self.ai2d
        del self.ai2d_builder
        del self.kpu
        del self.ai2d_input_tensor
        del self.ai2d_output_tensor
        gc.collect()
        nn.shrink_memory_pool()
        os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
        time.sleep_ms(100)