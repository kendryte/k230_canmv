import uctypes
from mpp import ai_def

def k_ai_chn_param(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(ai_def.k_ai_chn_param_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), ai_def.k_ai_chn_param_desc, layout)
    ai_def.k_ai_chn_param_parse(s, kwargs)
    return s

def k_ai_chn_pitch_shift_param(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(ai_def.k_ai_chn_pitch_shift_param_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), ai_def.k_ai_chn_pitch_shift_param_desc, layout)
    ai_def.k_ai_chn_pitch_shift_param_parse(s, kwargs)
    return s
