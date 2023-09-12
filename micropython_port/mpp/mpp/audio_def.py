import uctypes

k_audio_pdm_attr_desc = {
    "chn_cnt": 0 | uctypes.UINT32,
    "sample_rate": 4 | uctypes.UINT32,
    "bit_width": 8 | uctypes.UINT32,
    "snd_mode": 12 | uctypes.UINT32,
    "pdm_oversample": 16 | uctypes.UINT32,
    "frame_num": 20 | uctypes.UINT32,
    "point_num_per_frame": 24 | uctypes.UINT32,
}

k_audio_i2s_attr_desc = {
    "chn_cnt": 0 | uctypes.UINT32,
    "sample_rate": 4 | uctypes.UINT32,
    "bit_width": 8 | uctypes.UINT32,
    "snd_mode": 12 | uctypes.UINT32,
    "i2s_mode": 16 | uctypes.UINT32,
    "frame_num": 20 | uctypes.UINT32,
    "point_num_per_frame": 24 | uctypes.UINT32,
    "i2s_type": 28 | uctypes.UINT32,
}

kd_audio_attr_desc = {
    "pdm_attr": (0, k_audio_pdm_attr_desc),
    "i2s_attr": (0, k_audio_i2s_attr_desc),
}

k_aio_dev_attr_desc = {
    "audio_type": 0 | uctypes.UINT32,
    "avsync": 4 | uctypes.UINT32,
    "kd_audio_attr": (8, kd_audio_attr_desc),
}

def k_aio_dev_attr_parse(s, kwargs):
    s.audio_type = kwargs.get("audio_type", 0)
    s.avsync = kwargs.get("avsync", 0)

k_audio_frame_desc = {
    "bit_width": 0 | uctypes.UINT32,
    "snd_mode": 4 | uctypes.UINT32,
    "virt_addr": 8 | uctypes.UINT64,
    "phys_addr": 16 | uctypes.UINT64,
    "time_stamp": 24 | uctypes.UINT64,
    "seq": 32 | uctypes.UINT32,
    "len": 36 | uctypes.UINT32,
    "pool_id": 40 | uctypes.UINT32,
}

def k_audio_frame_parse(s, kwargs):
    s.bit_width = kwargs.get("bit_width", 0)
    s.snd_mode = kwargs.get("snd_mode", 0)
    s.virt_addr = kwargs.get("virt_addr", 0)
    s.phys_addr = kwargs.get("phys_addr", 0)
    s.time_stamp = kwargs.get("time_stamp", 0)
    s.seq = kwargs.get("seq", 0)
    s.len = kwargs.get("len", 0)
    s.pool_id = kwargs.get("pool_id", 0)

k_audio_stream_desc = {
    "stream": 0 | uctypes.UINT64,
    "phys_addr": 8 | uctypes.UINT64,
    "len": 16 | uctypes.UINT32,
    "time_stamp": 24 | uctypes.UINT64,
    "seq": 32 | uctypes.UINT32,
}

def k_audio_stream_parse(s, kwargs):
    s.stream = kwargs.get("stream", 0)
    s.phys_addr = kwargs.get("phys_addr", 0)
    s.len = kwargs.get("len", 0)
    s.time_stamp = kwargs.get("time_stamp", 0)
    s.seq = kwargs.get("seq", 0)
