import uctypes
from mpp import ao_def

def k_ao_chn_param(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(ao_def.k_ao_chn_param_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), ao_def.k_ao_chn_param_desc, layout)
    ao_def.k_ao_chn_param_parse(s, kwargs)
    return s
