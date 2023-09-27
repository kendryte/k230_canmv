import uctypes

k_mp4_config_muxer_s_desc = {
    "file_name": (0 | uctypes.ARRAY, 128 | uctypes.UINT8),
    "fmp4_flag": 128 | uctypes.UINT32,
}

def k_mp4_config_muxer_s_parse(s, kwargs):
    s.file_name[:] = kwargs.get("file_name", "").encode()
    s.fmp4_flag = kwargs.get("fmp4_flag", 0)

k_mp4_config_demuxer_s_desc = {
    "file_name": (0 | uctypes.ARRAY, 128 | uctypes.UINT8),
}

def k_mp4_config_demuxer_s_parse(s, kwargs):
    s.file_name[:] = kwargs.get("file_name", "").encode()

k_mp4_config_s_desc = {
    "config_type": 0 | uctypes.UINT32,
    "muxer_config": (4, k_mp4_config_muxer_s_desc),
    "demuxer_config": (4, k_mp4_config_demuxer_s_desc),
}

def k_mp4_config_s_parse(s, kwargs):
    s.config_type = kwargs.get("config_type", 0)

k_mp4_video_info_s_desc = {
    "width": 0 | uctypes.UINT32,
    "height": 4 | uctypes.UINT32,
    "track_id": 8 | uctypes.UINT32,
    "codec_id": 12 | uctypes.UINT32,
}

def k_mp4_video_info_s_parse(s, kwargs):
    s.width = kwargs.get("width", 0)
    s.height = kwargs.get("height", 0)
    s.track_id = kwargs.get("track_id", 0)
    s.codec_id = kwargs.get("codec_id", 0)

k_mp4_audio_info_s_desc = {
    "channels": 0 | uctypes.UINT32,
    "sample_rate": 4 | uctypes.UINT32,
    "bit_per_sample": 8 | uctypes.UINT32,
    "track_id": 12 | uctypes.UINT32,
    "codec_id": 16 | uctypes.UINT32
}

def k_mp4_audio_info_s_parse(s, kwargs):
    s.channels = kwargs.get("channels", 0)
    s.sample_rate = kwargs.get("sample_rate", 0)
    s.bit_per_sample = kwargs.get("bit_per_sample", 0)
    s.track_id = kwargs.get("track_id", 0)
    s.codec_id = kwargs.get("codec_id", 0)

k_mp4_track_info_s_desc = {
    "track_type": 0 | uctypes.UINT32,
    "time_scale": 4 | uctypes.UINT32,
    "video_info": (8, k_mp4_video_info_s_desc),
    "audio_info": (8, k_mp4_audio_info_s_desc),
}

def k_mp4_track_info_s_parse(s, kwargs):
    s.track_type = kwargs.get("track_type", 0)
    s.time_scale = kwargs.get("time_scale", 0)

k_mp4_file_info_s_desc = {
    "duration": 0 | uctypes.UINT64,
    "track_num": 8 | uctypes.UINT32,
}

def k_mp4_file_info_s_parse(s, kwargs):
    s.duration = kwargs.get("duration", 0)
    s.track_num = kwargs.get("track_num", 0)

k_mp4_frame_data_s_desc = {
    "codec_id": 0 | uctypes.UINT32,
    "time_stamp": 8 | uctypes.UINT64,
    "data": 16 | uctypes.UINT64,
    "data_length": 24 | uctypes.UINT32,
    "eof": 28 | uctypes.UINT32
}

def k_mp4_frame_data_s_parse(s, kwargs):
    s.codec_id = kwargs.get("codec_id", 0)
    s.time_stamp = kwargs.get("time_stamp", 0)
    s.data = kwargs.get("data", 0)
    s.data_length = kwargs.get("data_length", 0)
    s.eof = kwargs.get("eof", 0)