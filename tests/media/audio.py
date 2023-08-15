from media.pyaudio import *
import media.wave as wave
from media.media import *

def record_audio(filename, duration):
    CHUNK = int(44100/25)
    FORMAT = paInt16
    CHANNELS = 2
    RATE = 44100

    p = PyAudio()
    p.initialize(CHUNK)
    ret = media.buffer_init()
    if ret:
        print("record_audio, buffer_init failed")

    stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    frames = []

    for i in range(0, int(RATE / CHUNK * duration)):
        data = stream.read()
        frames.append(data)

    stream.stop_stream()
    stream.close()
    p.terminate()

    wf = wave.open(filename, 'wb')
    wf.set_channels(CHANNELS)
    wf.set_sampwidth(p.get_sample_size(FORMAT))
    wf.set_framerate(RATE)
    wf.write_frames(b''.join(frames))
    wf.close()

    media.buffer_deinit()

def play_audio(filename):
    #读取音频文件
    wf = wave.open(filename, 'rb')
    CHUNK = int(wf.get_framerate()/25)

    #播放音频操作
    p = PyAudio()
    p.initialize(CHUNK)
    ret = media.buffer_init()
    if ret:
        print("play_audio, buffer_init failed")

    stream = p.open(format=p.get_format_from_width(wf.get_sampwidth()),
                channels=wf.get_channels(),
                rate=wf.get_framerate(),
                output=True,frames_per_buffer=CHUNK)
    data = wf.read_frames(CHUNK)

    while data:
        stream.write(data)
        data = wf.read_frames(CHUNK)

    stream.stop_stream()
    stream.close()
    p.terminate()
    wf.close()

    media.buffer_deinit()


def loop_audio(duration):
    CHUNK = int(44100/25)
    FORMAT = paInt16
    CHANNELS = 2
    RATE = 44100

    p = PyAudio()
    p.initialize(CHUNK)
    ret = media.buffer_init()
    if ret:
        print("loop_audio, buffer_init failed")

    input_stream = p.open(format=FORMAT,
                    channels=CHANNELS,
                    rate=RATE,
                    input=True,
                    frames_per_buffer=CHUNK)

    output_stream = p.open(format=FORMAT,
                     channels=CHANNELS,
                     rate=RATE,
                     output=True,frames_per_buffer=CHUNK)

    for i in range(0, int(RATE / CHUNK * duration)):
        output_stream.write(input_stream.read())


    input_stream.stop_stream()
    output_stream.stop_stream()
    input_stream.close()
    output_stream.close()
    p.terminate()

    media.buffer_deinit()

#play_audio('/sdcard/app/input.wav')
#record_audio('/sdcard/app/output.wav', 15)
loop_audio(15)
print("audio sample done")
