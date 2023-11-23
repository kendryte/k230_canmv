import uctypes
from mpp import audio_def

KD_AUDIO_BIT_WIDTH_16 = const(0)
KD_AUDIO_BIT_WIDTH_24 = const(1)
KD_AUDIO_BIT_WIDTH_32 = const(2)

KD_AUDIO_SOUND_MODE_UNKNOWN = const(0)
KD_AUDIO_SOUND_MODE_MONO = const(1)
KD_AUDIO_SOUND_MODE_STEREO = const(2)

KD_I2S_IN_MONO_RIGHT_CHANNEL= const(0)
KD_I2S_IN_MONO_LEFT_CHANNEL = const(1)

KD_AUDIO_INPUT_TYPE_I2S = const(0)
KD_AUDIO_INPUT_TYPE_PDM = const(1)
KD_AUDIO_OUTPUT_TYPE_I2S = const(2)

KD_AUDIO_PDM_INPUT_OVERSAMPLE_32 = const(0)
KD_AUDIO_PDM_INPUT_OVERSAMPLE_64 = const(1)
KD_AUDIO_PDM_INPUT_OVERSAMPLE_128 = const(2)

K_STANDARD_MODE = const(1)
K_RIGHT_JUSTIFYING_MODE = const(2)
K_LEFT_JUSTIFYING_MODE = const(4)

K_AIO_I2STYPE_INNERCODEC = const(0)
K_AIO_I2STYPE_EXTERN = const(1)

def k_aio_dev_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(audio_def.k_aio_dev_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), audio_def.k_aio_dev_attr_desc, layout)
    audio_def.k_aio_dev_attr_parse(s, kwargs)
    return s

def k_audio_frame(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(audio_def.k_audio_frame_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), audio_def.k_audio_frame_desc, layout)
    audio_def.k_audio_frame_parse(s, kwargs)
    return s

def k_audio_stream(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(audio_def.k_audio_stream_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), audio_def.k_audio_stream_desc, layout)
    audio_def.k_audio_stream_parse(s, kwargs)
    return s
