from libs.PipeLine import ScopedTiming
import nncase_runtime as nn
import ulab.numpy as np

class Ai2d:
    def __init__(self,debug_mode=0):
        # 预处理ai2d
        self.ai2d=nn.ai2d()
        # ai2d计算过程中的输入输出数据类型，输入输出数据格式
        # self.ai2d.set_dtype(nn.ai2d_format.NCHW_FMT,nn.ai2d_format.NCHW_FMT,np.uint8, np.uint8)
        # ai2d构造器
        self.ai2d_builder=None
        # ai2d输入tensor对象
        self.ai2d_input_tensor=None
        # ai2d输出tensor对象
        self.ai2d_output_tensor=None
        self.debug_mode=debug_mode

    # 设置ai2d计算过程中的输入输出数据类型，输入输出数据格式
    def set_ai2d_dtype(self,input_format,output_format,input_type,output_type):
        self.ai2d.set_dtype(input_format,output_format,input_type,output_type)

    # 预处理crop函数
    # start_x：宽度方向的起始像素,int类型
    # start_y: 高度方向的起始像素,int类型
    # width: 宽度方向的crop长度,int类型
    # height: 高度方向的crop长度,int类型
    def crop(self,start_x,start_y,width,height):
        with ScopedTiming("init ai2d crop",self.debug_mode > 0):
            self.ai2d.set_crop_param(True,start_x,start_y,width,height)

    # 预处理shift函数
    # shift_val:右移的比特数,int类型
    def shift(self,shift_val):
        with ScopedTiming("init ai2d shift",self.debug_mode > 0):
            self.ai2d.set_shift_param(True,shift_val)

    # 预处理pad函数
    # paddings:各个维度的padding, size=8，分别表示dim0到dim4的前后padding的个数，其中dim0/dim1固定配置{0, 0},list类型
    # pad_mode:只支持pad constant，配置0即可,int类型
    # pad_val:每个channel的padding value,list类型
    def pad(self,paddings,pad_mode,pad_val):
        with ScopedTiming("init ai2d pad",self.debug_mode > 0):
            self.ai2d.set_pad_param(True,paddings,pad_mode,pad_val)

    # 预处理resize函数
    # interp_method:resize插值方法，ai2d_interp_method类型
    # interp_mode:resize模式，ai2d_interp_mode类型
    def resize(self,interp_method,interp_mode):
        with ScopedTiming("init ai2d resize",self.debug_mode > 0):
            self.ai2d.set_resize_param(True,interp_method,interp_mode)

    # 预处理affine函数
    # interp_method:Affine采用的插值方法,ai2d_interp_method类型
    # cord_round:整数边界0或者1,uint32_t类型
    # bound_ind:边界像素模式0或者1,uint32_t类型
    # bound_val:边界填充值,uint32_t类型
    # bound_smooth:边界平滑0或者1,uint32_t类型
    # M:仿射变换矩阵对应的vector，仿射变换为Y=[a_0, a_1; a_2, a_3] \cdot  X + [b_0, b_1] $, 则  M=[a_0,a_1,b_0,a_2,a_3,b_1 ],list类型
    def affine(self,interp_method,crop_round,bound_ind,bound_val,bound_smooth,M):
        with ScopedTiming("init ai2d affine",self.debug_mode > 0):
            self.ai2d.set_affine_param(True,interp_method,crop_round,bound_ind,bound_val,bound_smooth,M)

    # 构造ai2d预处理器
    def build(self,ai2d_input_shape,ai2d_output_shape,input_np=None):
        with ScopedTiming("ai2d build",self.debug_mode > 0):
            # ai2d构造函数
            self.ai2d_builder = self.ai2d.build(ai2d_input_shape, ai2d_output_shape)
            # 定义ai2d输出数据(即kmodel的输入数据，所以数据分辨率和模型的input_size一致)，并转换成tensor
            output_data = np.ones((ai2d_output_shape[0],ai2d_output_shape[1],ai2d_output_shape[2],ai2d_output_shape[3]),dtype=np.uint8)
            self.ai2d_output_tensor = nn.from_numpy(output_data)

    # 使用ai2d完成预处理
    def run(self,input_np):
        with ScopedTiming("ai2d run",self.debug_mode > 0):
            self.ai2d_input_tensor = nn.from_numpy(input_np)
            # 运行ai2d做初始化
            self.ai2d_builder.run(self.ai2d_input_tensor, self.ai2d_output_tensor)
            return self.ai2d_output_tensor
