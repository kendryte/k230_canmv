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
import image
import aicube
import random
import gc
import sys

# 自定义OCR检测任务类
class OCRDetectionApp(AIBase):
    def __init__(self,det_kmodel,model_input_size,mask_threshold=0.5,box_threshold=0.5,rgb888p_size=[1280,720],display_size=[1920,1080],debug_mode=0):
        super().__init__(det_kmodel,model_input_size,rgb888p_size,debug_mode)
        # kmodel路径
        self.det_kmodel=det_kmodel
        # OCR检测模型输入分辨率[width,height]
        self.model_input_size=model_input_size
        # ocr检测输出feature map二值化阈值
        self.mask_threshold=mask_threshold
        # 检测框分数阈值
        self.box_threshold=box_threshold
        # sensor给到AI的图像分辨率，宽16字节对齐
        self.rgb888p_size=[ALIGN_UP(rgb888p_size[0],16),rgb888p_size[1]]
        # 视频输出VO分辨率，宽16字节对齐
        self.display_size=[ALIGN_UP(display_size[0],16),display_size[1]]
        # debug模式
        self.debug_mode=debug_mode
        # Ai2d实例，用于实现模型预处理
        self.ai2d=Ai2d(debug_mode)
        # 设置Ai2d的输入输出格式和类型
        self.ai2d.set_ai2d_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)

    # 配置预处理操作，这里使用了pad和resize，Ai2d支持crop/shift/pad/resize/affine，具体代码请打开/sdcard/app/libs/AI2D.py查看
    def config_preprocess(self,input_image_size=None):
        with ScopedTiming("set preprocess config",self.debug_mode > 0):
            # 初始化ai2d预处理配置，默认为sensor给到AI的尺寸，您可以通过设置input_image_size自行修改输入尺寸
            ai2d_input_size=input_image_size if input_image_size else self.rgb888p_size
            # 计算padding参数，设置padding预处理
            self.ai2d.pad(self.get_pad_param(self.model_input_size,self.rgb888p_size), 0, [0, 0, 0])
            # 设置resize预处理
            self.ai2d.resize(nn.interp_method.tf_bilinear, nn.interp_mode.half_pixel)
            # build预处理过程，参数为输入tensor的shape和输出tensor的shape
            self.ai2d.build([1,3,ai2d_input_size[1],ai2d_input_size[0]],[1,3,self.model_input_size[1],self.model_input_size[0]])

    # 自定义当前任务的后处理
    def postprocess(self,results):
        with ScopedTiming("postprocess",self.debug_mode > 0):
            # chw2hwc
            hwc_array=self.chw2hwc(self.cur_img)
            # det_boxes结构为[[crop_array_nhwc,[p1_x,p1_y,p2_x,p2_y,p3_x,p3_y,p4_x,p4_y]],...]，crop_array_nhwc是切割的检测框数据，后八个数据表示检测框的左上，右上，右下，左下的坐标
            det_boxes = aicube.ocr_post_process(results[0][:,:,:,0].reshape(-1), hwc_array.reshape(-1),self.model_input_size,self.rgb888p_size, self.mask_threshold, self.box_threshold)
            # 只取坐标值
            all_boxes_pos=[]
            for det_box in det_boxes:
                all_boxes_pos.append(det_box[1])
            return all_boxes_pos

    # 绘制推理结果
    def draw_result(self,pl,all_boxes_pos):
        pl.osd_img.clear()
        # 一次绘制四条边，得到文本检测的四边形，坐标需要从原图分辨率转换成显示分辨率
        for i in range(len(all_boxes_pos)):
            for j in range(4):
                x1=all_boxes_pos[i][2*j]*self.display_size[0]//self.rgb888p_size[0]
                y1=all_boxes_pos[i][2*j+1]*self.display_size[1]//self.rgb888p_size[1]
                x2=all_boxes_pos[i][(2*j+2)%8]*self.display_size[0]//self.rgb888p_size[0]
                y2=all_boxes_pos[i][(2*j+3)%8]*self.display_size[1]//self.rgb888p_size[1]
                pl.osd_img.draw_line(int(x1),int(y1),int(x2),int(y2),color=(255,255,0,0),thickness=4)

    # 计算padding参数
    def get_pad_param(self,out_img_size,input_img_size):
        dst_w, dst_h = out_img_size
        input_w, input_h = input_img_size
        ratio = min(dst_w / input_w, dst_h / input_h)
        new_w, new_h = int(input_w * ratio), int(input_h * ratio)
        dw, dh = (dst_w - new_w) / 2, (dst_h - new_h) / 2
        top, bottom = int(round(0)), int(round(dh * 2))
        left, right = int(round(0)), int(round(dw * 2))
        return [0, 0, 0, 0, top, bottom, left, right]

    # chw2hwc
    def chw2hwc(self,features):
        ori_shape = (features.shape[0], features.shape[1], features.shape[2])
        c_hw_ = features.reshape((ori_shape[0], ori_shape[1] * ori_shape[2]))
        hw_c_ = c_hw_.transpose()
        new_array = hw_c_.copy()
        hwc_array = new_array.reshape((ori_shape[1], ori_shape[2], ori_shape[0]))
        del c_hw_
        del hw_c_
        del new_array
        return hwc_array

if __name__=="__main__":
    # 添加显示模式，支持"hdmi"和"lcd"
    display_mode="hdmi"
    if display_mode=="hdmi":
        display_size=[1920,1080]
    else:
        display_size=[800,480]
    # OCR检测kmdoel路径
    det_kmodel_path="/sdcard/app/tests/ai_test_kmodel/ocr_det_int16.kmodel"
    # 初始化PipeLine，只关注传给AI的图像分辨率，显示的分辨率
    pl=PipeLine(rgb888p_size=[640,360],display_size=display_size,display_mode=display_mode)
    pl.create()
    # OCR检测类实例，关注模型输入分辨率，传给AI的图像分辨率，显示的分辨率
    ocr_det=OCRDetectionApp(det_kmodel_path,model_input_size=[640,640],rgb888p_size=[640,360],display_size=display_size,debug_mode=0)
    # 配置预处理过程
    ocr_det.config_preprocess()
    try:
        while True:
            os.exitpoint()
            with ScopedTiming("total",1):
                # 获取当前帧
                img=pl.get_frame()
                # 推理当前帧获得检测框坐标
                boxes=ocr_det.run(img)
                # 绘制文本检测框
                ocr_det.draw_result(pl,boxes)
                # 在osd上显示文本检测框
                pl.show_image()
                gc.collect()
    except Exception as e:
        sys.print_exception(e)
    finally:
        ocr_det.deinit()
        pl.destroy()

