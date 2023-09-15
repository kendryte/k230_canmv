import uctypes
from mpp import adec_def

K_ADEC_MODE_PACK = const(0)
K_ADEC_MODE_STREAM = const(1)

def k_adec_chn_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(adec_def.k_adec_chn_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), adec_def.k_adec_chn_attr_desc, layout)
    adec_def.k_adec_chn_attr_parse(s, kwargs)
    return s
