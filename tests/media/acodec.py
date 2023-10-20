from media.pyaudio import *
from media.media import *
import media.g711 as g711
from mpp.payload_struct import *

def encode_audio(filename, duration):
    CHUNK = int(44100/25)
    FORMAT = paInt16
    CHANNELS = 2
    RATE = 44100

    p = PyAudio()
    p.initialize(CHUNK)
    enc = g711.Encoder(K_PT_G711A,CHUNK)
    ret = media.buffer_init()
    if ret:
        print("record_audio, buffer_init failed")

    enc.create()
    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    frames = []

    for i in range(0, int(RATE / CHUNK * duration)):
        frame_data = stream.read()
        data = enc.encode(frame_data)
        frames.append(data)

    stream.stop_stream()
    stream.close()
    p.terminate()
    enc.destroy()

    with open(filename,mode='w') as wf:
        wf.write(b''.join(frames))

    media.buffer_deinit()

def decode_audio(filename):
    #读取音频文件
    wf = open(filename,mode='rb')
    FORMAT = paInt16
    CHANNELS = 2
    RATE = 44100
    CHUNK = int(RATE/25)

    #播放音频操作
    p = PyAudio()
    p.initialize(CHUNK)
    dec = g711.Decoder(K_PT_G711A,CHUNK)
    ret = media.buffer_init()
    if ret:
        print("play_audio, buffer_init failed")

    dec.create()

    stream = p.open(format=FORMAT,
                channels=CHANNELS,
                rate=RATE,
                output=True,
                frames_per_buffer=CHUNK)

    stream_len = CHUNK*CHANNELS*2//2
    stream_data = wf.read(stream_len)

    while stream_data:
        frame_data = dec.decode(stream_data)
        stream.write(frame_data)
        stream_data = wf.read(stream_len)

    stream.stop_stream()
    stream.close()
    p.terminate()
    dec.destroy()
    wf.close()

    media.buffer_deinit()

def loop_codec(duration):
    CHUNK = int(44100/25)
    FORMAT = paInt16
    CHANNELS = 2
    RATE = 44100

    p = PyAudio()
    p.initialize(CHUNK)
    dec = g711.Decoder(K_PT_G711A,CHUNK)
    enc = g711.Encoder(K_PT_G711A,CHUNK)
    ret = media.buffer_init()
    if ret:
        print("loop_audio, buffer_init failed")

    dec.create()
    enc.create()

    input_stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    output_stream = p.open(format=FORMAT,
                     channels=CHANNELS,
                     rate=RATE,
                     output=True,
                     frames_per_buffer=CHUNK)

    for i in range(0, int(RATE / CHUNK * duration)):
        frame_data = input_stream.read()
        stream_data = enc.encode(frame_data)
        frame_data = dec.decode(stream_data)
        output_stream.write(frame_data)


    input_stream.stop_stream()
    output_stream.stop_stream()
    input_stream.close()
    output_stream.close()
    p.terminate()
    dec.destroy()
    enc.destroy()

    media.buffer_deinit()

#encode_audio('/sdcard/app/test.g711a', 15)
#decode_audio('/sdcard/app/test.g711a')
loop_codec(15)
print("audio codec sample done")
