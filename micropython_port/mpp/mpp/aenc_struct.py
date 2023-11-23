import uctypes
from mpp import aenc_def

AENC_MAX_DEV_NUMS = const(1)
AENC_MAX_CHN_NUMS = const(8)
def k_aenc_chn_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(aenc_def.k_aenc_chn_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), aenc_def.k_aenc_chn_attr_desc, layout)
    aenc_def.k_aenc_chn_attr_parse(s, kwargs)
    return s
