from libs.PipeLine import PipeLine, ScopedTiming
from libs.AIBase import AIBase
from libs.AI2D import Ai2d
import os
import ujson
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

# 自定义多标签分类任务类
class MultiLabelApp(AIBase):
    def __init__(self,kmodel_path,labels,model_input_size=[224,224],confidence_threshold=0.5,rgb888p_size=[224,224],display_size=[1920,1080],debug_mode=0):
        super().__init__(kmodel_path,model_input_size,rgb888p_size,debug_mode)
        self.kmodel_path=kmodel_path
        # 分类标签
        self.labels=labels
        # 模型输入分辨率
        self.model_input_size=model_input_size
        # 分类阈值
        self.confidence_threshold=confidence_threshold
        # sensor给到AI的图像分辨率，宽16字节对齐
        self.rgb888p_size=[ALIGN_UP(rgb888p_size[0],16),rgb888p_size[1]]
        # 显示分辨率，宽16字节对齐
        self.display_size=[ALIGN_UP(display_size[0],16),display_size[1]]
        self.debug_mode=debug_mode
        # Ai2d实例，用于实现模型预处理
        self.ai2d=Ai2d(debug_mode)
        # 设置Ai2d的输入输出格式和类型
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)

    # 配置预处理操作，这里使用了resize，Ai2d支持crop/shift/pad/resize/affine，具体代码请打开/sdcard/app/libs/AI2D.py查看
    def config_preprocess(self,input_image_size=None):
        with ScopedTiming("set preprocess config",self.debug_mode > 0):
            # 初始化ai2d预处理配置，默认为sensor给到AI的尺寸，您可以通过设置input_image_size自行修改输入尺寸
            ai2d_input_size=input_image_size if input_image_size else self.rgb888p_size
            # 配置resize预处理
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            # build预处理过程，参数为输入tensor的shape和输出tensor的shape
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]],[1,3,self.model_input_size[1],self.model_input_size[0]])

    # 自定义当前任务的后处理
    def postprocess(self,results):
        with ScopedTiming("postprocess",self.debug_mode > 0):
            res=[]
            scores=[]
            # 依次计算所有类别中的所属类别，对每一个类别做二分类
            for i in range(len(self.labels)):
                score=self.sigmoid(results[0][0][i])
                if score>self.confidence_threshold:
                   res.append(i)
                   scores.append(score)
            return res,scores

    # 将结果绘制到屏幕上
    def draw_result(self,pl,res,scores):
        with ScopedTiming("draw osd",self.debug_mode > 0):
            pl.osd_img.clear()
            mes=""
            # 组织多标签分类结果
            for i in range(len(res)):
                mes+=self.labels[res[i]]+" "+str(scores[i])+"\n"
            pl.osd_img.draw_string_advanced(5,5,32,mes,color=(255,0,255,0))

    # sigmoid函数
    def sigmoid(self,x):
        return 1.0 / (1.0 + np.exp(-x))


if __name__=="__main__":
    # 添加显示模式，支持"hdmi"和"lcd"
    display_mode="hdmi"
    if display_mode=="hdmi":
        display_size=[1920,1080]
    else:
        display_size=[800,480]
    # 模型路径，需要用户自行拷贝到开发板的目录下
    kmodel_path="/sdcard/app/tests/ai_test_kmodel/landscape_multilabel.kmodel"
    # 根据数据集设置，在线训练平台和AICube部署包的deploy_config.json文件中包含该字段
    labels=["沙漠","山脉","海洋","阳光","树"]
    # 初始化PipeLine
    pl=PipeLine(rgb888p_size=[1280,720],display_size=display_size,display_mode=display_mode)
    pl.create()
    # 初始化自定义多标签分类器
    multi=MultiLabelApp(kmodel_path,labels,rgb888p_size=[1280,720],display_size=display_size)
    # 配置预处理过程
    multi.config_preprocess()
    try:
        while True:
            os.exitpoint()
            with ScopedTiming("total",1):
                # 获取当前帧数据
                img=pl.get_frame()
                # 推理当前帧
                res,scores=multi.run(img)
                # 绘制结果到PipeLine的osd图像
                multi.draw_result(pl,res,scores)
                # 显示当前的绘制结果
                pl.show_image()
                gc.collect()
    except Exception as e:
        sys.print_exception(e)
    finally:
        multi.deinit()
        pl.destroy()
