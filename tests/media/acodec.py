# g711 encode/decode example
#
# Note: You will need an SD card to run this example.
#
# You can collect raw data and encode it into g711 or decode it into raw data output.


from media.pyaudio import * #导入pyaudio模块，用于采集和播放音频
from media.media import * #导入media模块，用于初始化vb buffer
import media.g711 as g711 #导入g711模块，用于g711编解码
from mpp.payload_struct import * #导入payload模块，用于获取音视频编解码类型

def encode_audio(filename, duration):
    CHUNK = int(44100/25) #设置音频chunk值
    FORMAT = paInt16 #设置采样精度
    CHANNELS = 2 #设置声道数
    RATE = 44100 #设置采样率

    p = PyAudio()
    p.initialize(CHUNK) #初始化PyAudio对象
    enc = g711.Encoder(K_PT_G711A,CHUNK) #创建g711编码器对象
    ret = media.buffer_init() #vb buffer初始化
    if ret:
        print("record_audio, buffer_init failed")

    enc.create() #创建编码器
    #创建音频输入流
    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    frames = []
    #采集音频数据编码并存入列表
    for i in range(0, int(RATE / CHUNK * duration)):
        frame_data = stream.read() #从音频输入流中读取音频数据
        data = enc.encode(frame_data) #编码音频数据为g711
        frames.append(data)  #将g711编码数据保存到列表中

    stream.stop_stream() #停止音频输入流
    stream.close() #关闭音频输入流
    p.terminate() #释放音频对象
    enc.destroy() #销毁g711音频编码器

    #将g711编码数据存入文件中
    with open(filename,mode='w') as wf:
        wf.write(b''.join(frames))

    media.buffer_deinit() #释放vb buffer

def decode_audio(filename):
    wf = open(filename,mode='rb') #打开g711文件
    FORMAT = paInt16 #设置音频chunk值
    CHANNELS = 2 #设置声道数
    RATE = 44100 #设置采样率
    CHUNK = int(RATE/25) #设置音频chunk值

    p = PyAudio()
    p.initialize(CHUNK) #初始化PyAudio对象
    dec = g711.Decoder(K_PT_G711A,CHUNK) #创建g711解码器对象
    ret = media.buffer_init() #vb buffer初始化
    if ret:
        print("play_audio, buffer_init failed")

    dec.create() #创建解码器

    #创建音频输出流
    stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                output=True,
                frames_per_buffer=CHUNK)

    stream_len = CHUNK*CHANNELS*2//2  #设置每次读取的g711数据流长度
    stream_data = wf.read(stream_len) #从g711文件中读取数据

    #解码g711文件并播放
    while stream_data:
        frame_data = dec.decode(stream_data) #解码g711文件
        stream.write(frame_data) #播放raw数据
        stream_data = wf.read(stream_len) #从g711文件中读取数据

    stream.stop_stream() #停止音频输入流
    stream.close() #关闭音频输入流
    p.terminate() #释放音频对象
    dec.destroy() #销毁解码器
    wf.close() #关闭g711文件

    media.buffer_deinit() #释放vb buffer

def loop_codec(duration):
    CHUNK = int(44100/25) #设置音频chunk值
    FORMAT = paInt16 #设置采样精度
    CHANNELS = 2 #设置声道数
    RATE = 44100 #设置采样率

    p = PyAudio()
    p.initialize(CHUNK) #初始化PyAudio对象
    dec = g711.Decoder(K_PT_G711A,CHUNK) #创建g711解码器对象
    enc = g711.Encoder(K_PT_G711A,CHUNK) #创建g711编码器对象
    ret = media.buffer_init() #vb buffer初始化
    if ret:
        print("loop_audio, buffer_init failed")

    dec.create() #创建g711解码器
    enc.create() #创建g711编码器

    #创建音频输入流
    input_stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    #创建音频输出流
    output_stream = p.open(format=FORMAT,
                     channels=CHANNELS,
                     rate=RATE,
                     output=True,
                     frames_per_buffer=CHUNK)

    #从音频输入流中获取数据->编码->解码->写入到音频输出流中
    for i in range(0, int(RATE / CHUNK * duration)):
        frame_data = input_stream.read() #从音频输入流中获取raw音频数据
        stream_data = enc.encode(frame_data) #编码音频数据为g711
        frame_data = dec.decode(stream_data) #解码g711数据为raw数据
        output_stream.write(frame_data) #播放raw数据


    input_stream.stop_stream() #停止音频输入流
    output_stream.stop_stream() #停止音频输出流
    input_stream.close() #关闭音频输入流
    output_stream.close() #关闭音频输出流
    p.terminate() #释放音频对象
    dec.destroy() #销毁g711解码器
    enc.destroy() #销毁g711编码器

    media.buffer_deinit() #释放vb buffer

#encode_audio('/sdcard/app/test.g711a', 15) #采集并编码g711文件
#decode_audio('/sdcard/app/test.g711a') #解码g711文件并输出
loop_codec(15) #采集音频数据->编码g711->解码g711->播放音频
print("audio codec sample done")
