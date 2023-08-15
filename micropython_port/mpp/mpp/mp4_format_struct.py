import uctypes
from mpp import mp4_format_def

K_MP4_CONFIG_MUXER = const(1)
K_MP4_CONFIG_DEMUXER = const(2)
K_MP4_CONFIG_BUTT = const(3)

K_MP4_STREAM_VIDEO = const(1)
K_MP4_STREAM_AUDIO = const(2)
K_MP4_STREAM_BUTT = const(3)

K_MP4_CODEC_ID_H264 = const(0)
K_MP4_CODEC_ID_H265 = const(1)
K_MP4_CODEC_ID_G711A = const(2)
K_MP4_CODEC_ID_G711U = const(3)
K_MP4_CODEC_ID_BUTT = const(4)

def k_mp4_config_muxer_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_config_muxer_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_config_muxer_s_desc, layout)
    mp4_format_def.k_mp4_config_muxer_s_parse(s, kwargs)
    return s

def k_mp4_config_demuxer_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_config_demuxer_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_config_demuxer_s_desc, layout)
    mp4_format_def.k_mp4_config_demuxer_s_parse(s, kwargs)
    return s

def k_mp4_config_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_config_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_config_s_desc, layout)
    mp4_format_def.k_mp4_config_s_parse(s, kwargs)
    return s

def k_mp4_video_info_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_video_info_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_video_info_s_desc, layout)
    mp4_format_def.k_mp4_video_info_s_parse(s, kwargs)
    return s

def k_mp4_audio_info_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_audio_info_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_audio_info_s_desc, layout)
    mp4_format_def.k_mp4_audio_info_s_parse(s, kwargs)
    return s

def k_mp4_track_info_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_track_info_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_track_info_s_desc, layout)
    mp4_format_def.k_mp4_track_info_s_parse(s, kwargs)
    return s

def k_mp4_file_info_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_file_info_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_file_info_s_desc, layout)
    mp4_format_def.k_mp4_file_info_s_parse(s, kwargs)
    return s

def k_mp4_frame_data_s(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(mp4_format_def.k_mp4_frame_data_s_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), mp4_format_def.k_mp4_frame_data_s_desc, layout)
    mp4_format_def.k_mp4_frame_data_s_parse(s, kwargs)
    return s