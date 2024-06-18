import uctypes

k_adec_chn_attr_desc = {
    "type": 0 | uctypes.UINT32,
    "mode": 4 | uctypes.UINT32,
    "point_num_per_frame": 8 | uctypes.UINT32,
    "buf_size": 12 | uctypes.UINT32,
}

def k_adec_chn_attr_parse(s, kwargs):
    s.type = kwargs.get("type", 0)
    s.mode = kwargs.get("mode", 0)
    s.point_num_per_frame = kwargs.get("point_num_per_frame", 0)
    s.buf_size = kwargs.get("buf_size", 0)
