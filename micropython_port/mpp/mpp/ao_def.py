import uctypes

k_ao_chn_param_desc = {
    "usr_frame_depth": 0 | uctypes.UINT32,
}

def k_ao_chn_param_parse(s, kwargs):
    s.usr_frame_depth = kwargs.get("usr_frame_depth", 0)
