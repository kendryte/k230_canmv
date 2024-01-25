from media.pyaudio import *                     # 音频模块
from media.media import *                       # 软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import media.wave as wave                       # wav音频处理模块
import nncase_runtime as nn                     # nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
import ulab.numpy as np                         # 类似python numpy操作，但也会有一些接口不同
import aidemo                                   # aidemo模块，封装ai demo相关前处理、后处理等操作
import time                                     # 时间统计
import struct                                   # 字节字符转换模块
import gc                                       # 垃圾回收模块
import os,sys                                   # 操作系统接口模块

# key word spotting任务
# 检测阈值
THRESH = 0.5
# 有关音频流的宏变量
SAMPLE_RATE = 16000         # 采样率16000Hz,即每秒采样16000次
CHANNELS = 1                # 通道数 1为单声道，2为立体声
FORMAT = paInt16            # 音频输入输出格式 paInt16
CHUNK = int(0.3 * 16000)    # 每次读取音频数据的帧数，设置为0.3s的帧数16000*0.3=4800

root_dir='/sdcard/app/tests/'
kmodel_file_kws = root_dir+"kmodel/kws.kmodel"      # kmodel加载路径
reply_wav_file = root_dir+"utils/wozai.wav"         # kws唤醒词回复音频路径
debug_mode = 0                                      # 调试模式，大于0（调试）、 反之 （不调试）


# scoped_timing.py 用于debug模式输出程序块运行时间
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

# 当前kmodel
global current_kmodel_obj                                                               # 定义全局kpu对象
global p,cache_np,fp,input_stream,output_stream,audio_input_tensor,cache_input_tensor   # 定义全局音频流对象，输入输出流对象，并且定义kws处理接口FeaturePipeline对象fp,输入输出tensor和缓冲cache_np

# 初始化kws音频流相关变量
def init_kws():
    with ScopedTiming("init_kws",debug_mode > 0):
        global p,cache_np,fp,input_stream,output_stream,cache_input_tensor
        # 初始化模型的cache输入
        cache_np = np.zeros((1, 256, 105), dtype=np.float)
        cache_input_tensor = nn.from_numpy(cache_np)
        # 初始化音频预处理接口
        fp = aidemo.kws_fp_create()
        # 初始化音频流
        p = PyAudio()
        p.initialize(CHUNK)
        media.buffer_init()
        # 用于采集实时音频数据
        input_stream = p.open(
                        format=FORMAT,
                        channels=CHANNELS,
                        rate=SAMPLE_RATE,
                        input=True,
                        frames_per_buffer=CHUNK
                        )

        # 用于播放回复音频
        output_stream = p.open(
                        format=FORMAT,
                        channels=CHANNELS,
                        rate=SAMPLE_RATE,
                        output=True,
                        frames_per_buffer=CHUNK
                        )

# kws 初始化kpu
def kpu_init_kws():
    with ScopedTiming("init_kpu",debug_mode > 0):
        # 初始化kpu并加载kmodel
        kpu = nn.kpu()
        kpu.load_kmodel(kmodel_file_kws)
        return kpu

# kws 释放kpu
def kpu_deinit():
    # kpu释放
    with ScopedTiming("kpu_deinit",debug_mode > 0):
        global current_kmodel_obj,audio_input_tensor,cache_input_tensor
        if "current_kmodel_obj" in globals():
            del current_kmodel_obj
        if "audio_input_tensor" in globals():
            del audio_input_tensor
        if "cache_input_tensor" in globals():
            del cache_input_tensor

# kws音频预处理
def kpu_pre_process_kws(pcm_data_list):
    global current_kmodel_obj
    global fp,input_stream,audio_input_tensor,cache_input_tensor
    with ScopedTiming("pre_process",debug_mode > 0):
        # 将pcm数据处理为模型输入的特征向量
        mp_feats = aidemo.kws_preprocess(fp, pcm_data_list)[0]
        mp_feats_np = np.array(mp_feats)
        mp_feats_np = mp_feats_np.reshape((1, 30, 40))
        audio_input_tensor = nn.from_numpy(mp_feats_np)
        cache_input_tensor = nn.from_numpy(cache_np)
        current_kmodel_obj.set_input_tensor(0, audio_input_tensor)
        current_kmodel_obj.set_input_tensor(1, cache_input_tensor)

# kws任务kpu运行并完成后处理
def kpu_run_kws(kpu_obj,pcm_data_list):
    global current_kmodel_obj,cache_np,output_stream
    current_kmodel_obj = kpu_obj
    # （1）kws音频数据预处理
    kpu_pre_process_kws(pcm_data_list)
    # （2）kpu推理
    with ScopedTiming("kpu_run",debug_mode > 0):
        kpu_obj.run()
    # （3）获取模型输出
    logits = kpu_obj.get_output_tensor(0)
    cache_tensor = kpu_obj.get_output_tensor(1)     # 更新缓存输入
    logits_np = logits.to_numpy()
    cache_np=cache_tensor.to_numpy()
    del logits
    del cache_tensor
    # （4）后处理argmax
    max_logits = np.max(logits_np, axis=1)[0]
    max_p = np.max(max_logits)
    idx = np.argmax(max_logits)
    # 如果分数大于阈值，且idx==1(即包含唤醒词)，播放回复音频
    if max_p > THRESH and idx == 1:
        print("====Detected XiaonanXiaonan!====")
        wf = wave.open(reply_wav_file, "rb")
        wav_data = wf.read_frames(CHUNK)
        while wav_data:
            output_stream.write(wav_data)
            wav_data = wf.read_frames(CHUNK)
        time.sleep(1) # 时间缓冲，用于播放声音
        wf.close()
    else:
        print("Deactivated!")


# kws推理过程
def kws_inference():
    # 记录音频帧帧数
    global p,fp,input_stream,output_stream,current_kmodel_obj
    # 初始化
    init_kws()
    kpu_kws=kpu_init_kws()
    pcm_data_list = []
    try:
        gc_count=0
        while True:
            os.exitpoint()
            with ScopedTiming("total", 1):
                pcm_data_list.clear()
                # 对实时音频流进行推理
                pcm_data = input_stream.read()  # 获取的音频流数据字节数，len(pcm_data)=0.3*16000*2=9600，即以16000Hz的采样率采样0.3s，每次采样数据为paInt16格式占2个字节
                # 获取音频流数据
                for i in range(0, len(pcm_data), 2):
                    # 每两个字节组织成一个有符号整数，然后将其转换为浮点数，即为一次采样的数据，加入到当前一帧（0.3s）的数据列表中
                    int_pcm_data = struct.unpack("<h", pcm_data[i:i+2])[0]
                    float_pcm_data = float(int_pcm_data)
                    pcm_data_list.append(float_pcm_data)
                # kpu运行和后处理
                kpu_run_kws(kpu_kws,pcm_data_list)
                if gc_count > 10:
                    gc.collect()
                    gc_count = 0
                else:
                    gc_count += 1
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        input_stream.stop_stream()
        output_stream.stop_stream()
        input_stream.close()
        output_stream.close()
        p.terminate()
        media.buffer_deinit()
        aidemo.kws_fp_destroy(fp)
        kpu_deinit()
        del kpu_kws
        if "current_kmodel_obj" in globals():
            del current_kmodel_obj
        gc.collect()
        nn.shrink_memory_pool()

if __name__=="__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    kws_inference()
