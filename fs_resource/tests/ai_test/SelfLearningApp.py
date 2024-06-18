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

# 自定义自学习任务类
class SelfLearningApp(AIBase):
    def __init__(self,kmodel_path,model_input_size=[224,224],confidence_threshold=0.5,rgb888p_size=[224,224],display_size=[1920,1080],debug_mode=0):
        super().__init__(kmodel_path,model_input_size,rgb888p_size,debug_mode)
        # kmodel路径
        self.kmodel_path=kmodel_path
        # 模型输入分辨率
        self.model_input_size=model_input_size
        # sensor给到AI的图像分辨率，宽16字节对齐
        self.rgb888p_size=[ALIGN_UP(rgb888p_size[0],16),rgb888p_size[1]]
        # 视频输出VO分辨率，宽16字节对齐
        self.display_size=[ALIGN_UP(display_size[0],16),display_size[1]]
        # debug模式
        self.debug_mode=debug_mode
        # 模型输出列表
        self.results=[]
        # features库
        self.embeddings=[]
        # features对应的标签
        self.embeddings_labels=[]
        self.ai2d=Ai2d(debug_mode)
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)

    def config_preprocess(self,input_image_size=None):
        with ScopedTiming("set preprocess config",self.debug_mode > 0):
            # 初始化ai2d预处理配置
            ai2d_input_size=input_image_size if input_image_size else self.rgb888p_size
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            self.ai2d.build(ai2d_input_size,self.model_input_size)

    # 加载图片，将图片特征化后存入特征向量库
    def load_image(self,image_path,label):
        img=self.read_img(image_path)
        self.config_preprocess([img.shape[2],img.shape[1]])
        tensor=self.preprocess(img)
        results=self.inference([tensor])
        self.config_preprocess()
        self.embeddings.append(results[0][0])
        self.embeddings_labels.append(label)
        del img
        gc.collect()

    # 从本地读入图片，并实现HWC转CHW
    def read_img(self,img_path):
        img_data = image.Image(img_path)
        img_data_rgb888=img_data.to_rgb888()
        img_hwc=img_data_rgb888.to_numpy_ref()
        shape=img_hwc.shape
        img_tmp = img_hwc.reshape((shape[0] * shape[1], shape[2]))
        img_tmp_trans = img_tmp.transpose()
        img_res=img_tmp_trans.copy()
        img_return=img_res.reshape((shape[2],shape[0],shape[1]))
        return img_return

    # 自学习任务推理流程
    def postprocess(self,results):
        with ScopedTiming("postprocess",self.debug_mode > 0):
            if len(self.embeddings)>0:
                # 计算特征向量和向量库中所有向量的最大相似度和相似向量的索引
                idx,score=self.compute_similar(results[0][0])
                gc.collect()
                # 返回分类标签和分数
                if len(self.embeddings_labels):
                    return self.embeddings_labels[idx],score
                else:
                    return "", 0.0
            else:
                return "Please add new category images...", 0.0

    # 绘制分类结果
    def draw_result(self,pl,res,score):
        with ScopedTiming("draw osd",self.debug_mode > 0):
            pl.osd_img.clear()
            mes=res+" "+str(round(score,3))
            pl.osd_img.draw_string_advanced(5,5,32,mes,color=(255,0,255,0))

    # 计算参数向量和向量库所有向量的相似度，并返回最大相似索引和对应的相似度分数
    def compute_similar(self,embedding):
        output = np.linalg.norm(embedding)
        embed_lib = np.linalg.norm(np.array(self.embeddings,dtype=np.float), axis=1)
        dot_products = np.dot(np.array(self.embeddings), embedding)
        similarities = dot_products / (embed_lib * output)
        most_similar_index=np.argmax(similarities)
        return most_similar_index,similarities[most_similar_index]



if __name__=="__main__":
    display_mode="lcd"
    if display_mode=="hdmi":
        display_size=[1920,1080]
    else:
        display_size=[800,480]
    # kmodel模型路径
    kmodel_path="/sdcard/app/tests/ai_test_kmodel/embedding.kmodel"
    # 初始化PipeLine
    pl=PipeLine(rgb888p_size=[1280,720],display_size=display_size,display_mode=display_mode)
    pl.create()
    # 自定义自学习分类器实例化
    cls=SelfLearningApp(kmodel_path,model_input_size=[224,224],rgb888p_size=[1280,720],display_size=display_size)
    cls.config_preprocess()
    # 加载类别图片
    cls.load_image("/sdcard/app/tests/ai_test_utils/0.jpg","菠菜")
    cls.load_image("/sdcard/app/tests/ai_test_utils/1.jpg","菠菜")
    cls.load_image("/sdcard/app/tests/ai_test_utils/2.jpg","菠菜")

    cls.load_image("/sdcard/app/tests/ai_test_utils/3.jpg","长茄子")
    cls.load_image("/sdcard/app/tests/ai_test_utils/4.jpg","长茄子")
    cls.load_image("/sdcard/app/tests/ai_test_utils/5.jpg","长茄子")

    cls.load_image("/sdcard/app/tests/ai_test_utils/6.jpg","胡萝卜")
    cls.load_image("/sdcard/app/tests/ai_test_utils/7.jpg","胡萝卜")
    cls.load_image("/sdcard/app/tests/ai_test_utils/8.jpg","胡萝卜")
    try:
        while True:
            os.exitpoint()
            # 获取当前帧
            img=pl.get_frame()
            # 获取分类结果和分数
            res,score=cls.run(img)
            # 绘制结果到PipeLine的osd图像
            cls.draw_result(pl,res,score)
            # 显示当前的绘制结果
            pl.show_image()
    except Exception as e:
        sys.print_exception(e)
    finally:
        cls.deinit()
        pl.destroy()

