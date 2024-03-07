from media.pyaudio import *                 # 音频模块
from media.media import *                   # 软件抽象模块，主要封装媒体数据链路以及媒体缓冲区
import media.wave as wave                   # wav音频处理模块
import nncase_runtime as nn                 # nncase运行模块，封装了kpu（kmodel推理）和ai2d（图片预处理加速）操作
import ulab.numpy as np                     # 类似python numpy操作，但也会有一些接口不同
import time                                 # 时间统计
import aidemo                               # aidemo模块，封装ai demo相关前处理、后处理等操作
import struct                               # 字节字符转换模块
import gc                                   # 垃圾回收模块
import os,sys                               # 操作系统接口模块

# 有关音频流的宏变量
SAMPLE_RATE = 24000         # 采样率24000Hz,即每秒采样24000次
CHANNELS = 1                # 通道数 1为单声道，2为立体声
FORMAT = paInt16            # 音频输入输出格式 paInt16
CHUNK = int(0.3 * 24000)    # 每次读取音频数据的帧数，设置为0.3s的帧数24000*0.3=7200

# tts_zh（文字转语音中文）任务
root_dir='/sdcard/app/tests/'
# 拼音字典
dict_path=root_dir+"utils/pinyin.txt"
# 汉字转拼音字典文件
phase_path=root_dir+"utils/small_pinyin.txt"
# 拼音转音素映射文件
mapfile=root_dir+"utils/phone_map.txt"
# 输入中文语句
text="嘉楠科技研发了最新款的芯片"
# 中文tts encoder模型
fastspeech1_path=root_dir+"kmodel/zh_fastspeech_1_f32.kmodel"
# 中文tts decoder模型
fastspeech2_path=root_dir+"kmodel/zh_fastspeech_2.kmodel"
# 中文tts 声码器模型
hifigan_path=root_dir+"kmodel/hifigan.kmodel"
# 生成音频存储路径
save_wav_file = root_dir+"test.wav"
debug_mode=1


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

global ttszh

def play_audio():
    # 初始化音频流
    p = PyAudio()
    p.initialize(CHUNK)
    ret = media.buffer_init()
    if ret:
        print("record_audio, buffer_init failed")
    # 用于播放音频
    output_stream = p.open(
                    format=FORMAT,
                    channels=CHANNELS,
                    rate=SAMPLE_RATE,
                    output=True,
                    frames_per_buffer=CHUNK
                    )
    wf = wave.open(save_wav_file, "rb")
    wav_data = wf.read_frames(CHUNK)
    while wav_data:
        output_stream.write(wav_data)
        wav_data = wf.read_frames(CHUNK)
    time.sleep(2) # 时间缓冲，用于播放声音
    wf.close()
    output_stream.stop_stream()
    output_stream.close()
    p.terminate()
    media.buffer_deinit()

def ttszh_init():
    with ScopedTiming("init_ttszh",debug_mode > 0):
        global ttszh
        ttszh=aidemo.tts_zh_create(dict_path,phase_path,mapfile)

# ttszh推理过程
def ttszh_inference():
    global ttszh
    try:
        # ttszh初始化
        ttszh_init()
        with ScopedTiming("preprocess",debug_mode > 0):
            # ttszh数据预处理，获取data和data_len
            preprocess_data=aidemo.tts_zh_preprocess(ttszh,text)
            data=preprocess_data[0]
            data_len=preprocess_data[1]
        with ScopedTiming("encode run",debug_mode > 0):
            # 创建编码器kpu，加载kmodel
            kpu_enc = nn.kpu()
            kpu_enc.load_kmodel(fastspeech1_path)
            # 创建编码器模型输入并和模型绑定，编码器包含两个输入，一个是文字预处理的序列数据，一个是speaker数据
            # 编码器序列数据
            enc_seq_input_array=np.array(data)
            enc_seq_input_tensor = nn.from_numpy(enc_seq_input_array)
            # 编码器speaker数据
            enc_speaker_input_array=np.array([0.0])
            enc_speaker_input_tensor=nn.from_numpy(enc_speaker_input_array)
            # 和模型绑定
            kpu_enc.set_input_tensor(1, enc_seq_input_tensor)
            kpu_enc.set_input_tensor(0, enc_speaker_input_tensor)
            # kpu运行
            kpu_enc.run()
            # 获取kpu的输入
            enc_output_0=kpu_enc.get_output_tensor(0)
            enc_output_1=kpu_enc.get_output_tensor(1)
            enc_output_0_np=enc_output_0.to_numpy()
            enc_output_1_np=enc_output_1.to_numpy()
        with ScopedTiming("encode postprocess",debug_mode > 0):
            # 给编码结果添加持续时间属性，每个音素编码向量按照持续时间重复
            duritions=enc_output_1_np[0][:int(data_len[0])]
            durition_sum=int(np.sum(duritions))
            # 解码器输入维度为（1,600,256）,不足部分需要padding
            max_value=13
            while durition_sum>600:
                for i in range(len(duritions)):
                    if duritions[i]>max_value:
                        duritions[i]=max_value
                max_value=max_value-1
                durition_sum=np.sum(duritions)
            dec_input=np.zeros((1,600,256),dtype=np.float)
            m_pad=600-durition_sum
            k=0
            for i in range(len(duritions)):
                for j in range(int(duritions[i])):
                    dec_input[0][k]=enc_output_0_np[0][i]
                    k+=1
        with ScopedTiming("decode run",debug_mode > 0):
            # 定义解码器kpu对象，并加载kmodel
            kpu_dec = nn.kpu()
            kpu_dec.load_kmodel(fastspeech2_path)
            #设置解码器模型输入
            dec_input_tensor=nn.from_numpy(dec_input)
            kpu_dec.set_input_tensor(0, dec_input_tensor)
            # 运行
            kpu_dec.run()
            # 获取解码器模型输出，维度为（1，80，600）
            dec_output=kpu_dec.get_output_tensor(0)
            dec_output_np=dec_output.to_numpy()
        with ScopedTiming("decode postprocess",debug_mode > 0):
            # 将有效信息拆分成一个个（1，80，100）的子向量，输入声码器生成音频
            subvector_num=durition_sum//100;
            remaining=durition_sum%100;
            if remaining>0:
                subvector_num+=1
            hifi_input=np.zeros((1,80,subvector_num*100),dtype=np.float)
            for i in range(durition_sum):
                hifi_input[:,:,i]=dec_output_np[:,:,i]

        with ScopedTiming("hifigan run",debug_mode > 0):
            # 定义hifigan声码器模型kpu对象，加载kmodel
            kpu_hifi = nn.kpu()
            kpu_hifi.load_kmodel(hifigan_path)


            # 保存生成的所有梅尔声谱数据，后续保存成wav文件
            mel_data=[]
            # 依次对每一个子向量进行声码器推理
            for i in range(subvector_num):
                hifi_input_tmp=np.zeros((1,80,100),dtype=np.float)

                for j in range(80):
                    for k in range(i*100,(i+1)*100):
                        hifi_input_tmp[0][j][k-i*100]=hifi_input[0][j][k]
                # 设置模型输入
                hifigan_input_tensor=nn.from_numpy(hifi_input_tmp)
                kpu_hifi.set_input_tensor(0, hifigan_input_tensor)
                # kpu运行
                kpu_hifi.run()
                # 获取模型输出
                hifigan_output=kpu_hifi.get_output_tensor(0)
                hifigan_output_np=hifigan_output.to_numpy()
                # 汇总输出数据
                for j in range(25600):
                    mel_data.append(hifigan_output_np[0][0][j])
                del hifigan_input_tensor
        with ScopedTiming("save wav file",debug_mode > 0):
            # 将生成的音频数据保存为wav文件
            save_data=mel_data[:durition_sum*256]
            save_len=len(save_data)
            aidemo.save_wav(save_data,save_len,save_wav_file,SAMPLE_RATE)
        aidemo.tts_zh_destroy(ttszh)
        with ScopedTiming("play audio",debug_mode > 0):
            play_audio()
        del kpu_enc
        del kpu_dec
        del kpu_hifi
        del enc_seq_input_tensor
        del enc_speaker_input_tensor
        del enc_output_0
        del enc_output_1
        del dec_input_tensor
        del dec_output
        del ttszh
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)
    finally:
        gc.collect()


if __name__=="__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    nn.shrink_memory_pool()
    ttszh_inference()
