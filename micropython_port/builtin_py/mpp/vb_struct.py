import uctypes
from mpp import vb_def

VB_UID_VI = const(0)
VB_UID_VENC = const(1)
VB_UID_VDEC = const(2)
VB_UID_VO = const(3)
VB_UID_USER = const(4)
VB_UID_AI = const(5)
VB_UID_AREC = const(6)
VB_UID_AENC = const(7)
VB_UID_ADEC = const(8)
VB_UID_AO = const(9)
VB_UID_V_VI = const(10)
VB_UID_V_VO = const(11)
VB_UID_DMA  = const(12)
VB_UID_DPU  = const(13)
VB_UID_BUTT = const(14)

VB_REMAP_MODE_NONE = const(0)
VB_REMAP_MODE_NOCACHE = const(1)
VB_REMAP_MODE_CACHED = const(2)
VB_REMAP_MODE_BUTT = const(3)

VB_SUPPLEMENT_JPEG_MASK = const(1 << 0)
VB_SUPPLEMENT_ISP_INFO_MASK = const(1 << 1)
VB_SUPPLEMENT_UNSPPORT_MASK = const(~(VB_SUPPLEMENT_JPEG_MASK | VB_SUPPLEMENT_ISP_INFO_MASK))

def k_vb_pool_config(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_pool_config_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_pool_config_desc, layout)
    vb_def.k_vb_pool_config_parse(s, kwargs)
    return s

def k_vb_config(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_config_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_config_desc, layout)
    vb_def.k_vb_config_parse(s, kwargs)
    return s

def k_vb_mod_config(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_mod_config_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_mod_config_desc, layout)
    vb_def.k_vb_mod_config_parse(s, kwargs)
    return s

def k_vb_pool_info(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_pool_info_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_pool_info_desc, layout)
    vb_def.k_vb_pool_info_parse(s, kwargs)
    return s

def k_vb_supplement_config(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_supplement_config_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_supplement_config_desc, layout)
    vb_def.k_vb_supplement_config_parse(s, kwargs)
    return s

def k_vb_pool_status(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.k_vb_pool_status_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.k_vb_pool_status_desc, layout)
    vb_def.k_vb_pool_status_parse(s, kwargs)
    return s

def vb_block_info(**kwargs):
    layout= uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vb_def.vb_block_info_desc), layout)
    s = uctypes.struct(uctypes.addressof(buf), vb_def.vb_block_info_desc, layout)
    vb_def.vb_block_info_parse(s, kwargs)
    return s
