import uctypes
from mpp import vdec_def

VDEC_MAX_CHN_NUMS = const(4)

K_VDEC_SEND_MODE_STREAM = const(0)
K_VDEC_SEND_MODE_FRAME = const(1)
K_VDEC_SEND_MODE_BUTT = const(2)

K_VDEC_DSL_MODE_BY_SIZE = const(0)
K_VDEC_DSL_MODE_BY_RATIO = const(1)
K_VDEC_DSL_MODE_BUTT = const(2)

def k_vdec_chn_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_chn_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_chn_attr_desc, layout)
    vdec_def.k_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_supplement_info(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_supplement_info_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_supplement_info_desc, layout)
    vdec_def.k_vdec_supplement_info_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_stream(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_stream_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_stream_desc, layout)
    vdec_def.k_vdec_stream_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_dec_err(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_dec_err_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_dec_err_desc, layout)
    vdec_def.k_vdec_dec_err_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_chn_status(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_chn_status_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_chn_status_desc, layout)
    vdec_def.k_vdec_chn_status_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_dsl_size(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_dsl_size_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_dsl_size_desc, layout)
    vdec_def.k_vdec_dsl_size_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_dsl_ratio(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_dsl_ratio_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_dsl_ratio_desc, layout)
    vdec_def.k_vdec_dsl_ratio_pak_vdec_chn_attr_parse(s, kwargs)
    return s

def k_vdec_downscale(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vdec_def.k_vdec_downscale_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vdec_def.k_vdec_downscale_desc, layout)
    vdec_def.k_vdec_downscale_pak_vdec_chn_attr_parse(s, kwargs)
    return s
