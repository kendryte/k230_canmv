from libs.PipeLine import ScopedTiming
import os
import ujson
from media.sensor import *
from media.display import *
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
    def __init__(self,kmodel_path,model_input_size=None,rgb888p_size=None,debug_mode=0):
        # kmodel路径
        self.kmodel_path=kmodel_path
        # 模型输入分辨率
        self.model_input_size=model_input_size
        # sensor给到AI的图像分辨率
        self.rgb888p_size=rgb888p_size
        # 调试模式
        self.debug_mode=debug_mode
        # kpu对象
        self.kpu=nn.kpu()
        self.kpu.load_kmodel(self.kmodel_path)
        self.cur_img=None
        self.tensors=[]
        # 推理结果列表
        self.results=[]

    def get_kmodel_inputs_num(self):
        return self.kpu.inputs_size()
    
    def get_kmodel_outputs_num(self):
        return self.kpu.outputs_size()
    
    def preprocess(self,input_np):
        with ScopedTiming("preprocess",self.debug_mode > 0):
            return self.ai2d.run(input_np)

    def inference(self,tensors):
        with ScopedTiming("kpu run & get output",self.debug_mode > 0):
            self.results.clear()
            for i in range(self.kpu.inputs_size()):
                # 将ai2d的输出tensor绑定为kmodel的输入数据
                self.kpu.set_input_tensor(i, tensors[i])
            # 运行kmodel做推理
            self.kpu.run()
            # 获取kmodel的推理输出tensor,输出可能为多个，因此返回的是一个列表
            for i in range(self.kpu.outputs_size()):
                output_data = self.kpu.get_output_tensor(i)
                result = output_data.to_numpy()
                self.results.append(result)
                del output_data
            return self.results

    # 基类后处理接口
    def postprocess(self,results):
        return

    # kmodel运行pipe，包括预处理+推理+后处理，后处理在单独的任务类中实现
    def run(self,input_np):
        with ScopedTiming("total",self.debug_mode > 0):
            self.cur_img=input_np
            self.tensors.clear()
            self.tensors.append(self.preprocess(input_np))
            self.results=self.inference(self.tensors)
            return self.postprocess(self.results)

    # AIBase销毁函数
    def deinit(self):
        with ScopedTiming("deinit",self.debug_mode > 0):
            del self.kpu
            del self.ai2d
            self.tensors.clear()
            del self.tensors
            gc.collect()
            nn.shrink_memory_pool()
            os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
            time.sleep_ms(100)
