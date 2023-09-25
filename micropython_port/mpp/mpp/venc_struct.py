import uctypes
from mpp import venc_def

VENC_MAX_CHN_NUMS = const(4)
VENC_ALIGN_4K = const(0x1000)

VENC_PROFILE_NONE = const(0)
VENC_PROFILE_H264_BASELINE = const(1)
VENC_PROFILE_H264_MAIN = const(2)
VENC_PROFILE_H264_HIGH = const(3)
VENC_PROFILE_H264_HIGH_10 = const(4)
VENC_PROFILE_H265_MAIN = const(5)
VENC_PROFILE_H265_MAIN_10 = const(6)
VENC_PROFILE_BUTT = const(7)

K_VENC_RC_MODE_CBR = const(1)
K_VENC_RC_MODE_VBR = const(2)
K_VENC_RC_MODE_FIXQP = const(3)
K_VENC_RC_MODE_MJPEG_FIXQP = const(4)
K_VENC_RC_MODE_BUTT = const(5)

K_VENC_MIRROR_HORI = const(1)
K_VENC_MIRROR_VERT = const(2)
K_VENC_MIRROR_BUTT = const(3)

K_VENC_P_FRAME = const(1)
K_VENC_I_FRAME = const(2)
K_VENC_HEADER = const(3)
K_VENC_BUTT = const(4)

ENTROPY_MODE_CAVLC = const(0)
ENTROPY_MODE_CABAC = const(1)
ENTROPY_MODE_BUTT = const(2)

K_VENC_MAX_2D_OSD_REGION_NUM = const(8)
K_VENC_MAX_2D_BORDER_NUM = const(32)
K_VENC_2D_MAX_CHN_NUM = const(3)
K_VENC_2D_COEF_NUM = const(12)

K_VENC_2D_CALC_MODE_CSC = const(0)
K_VENC_2D_CALC_MODE_OSD = const(1)
K_VENC_2D_CALC_MODE_BORDER = const(2)
K_VENC_2D_CALC_MODE_OSD_BORDER = const(3)
K_VENC_2D_CALC_MODE_BUTT = const(4)

K_VENC_2D_SRC_DST_FMT_YUV420_NV12 = const(0)
K_VENC_2D_SRC_DST_FMT_YUV420_NV21 = const(1)
K_VENC_2D_SRC_DST_FMT_YUV420_I420 = const(2)
K_VENC_2D_SRC_DST_FMT_ARGB8888 = const(4)
K_VENC_2D_SRC_DST_FMT_ARGB4444 = const(5)
K_VENC_2D_SRC_DST_FMT_ARGB1555 = const(6)
K_VENC_2D_SRC_DST_FMT_XRGB8888 = const(7)
K_VENC_2D_SRC_DST_FMT_XRGB4444 = const(8)
K_VENC_2D_SRC_DST_FMT_XRGB1555 = const(9)
K_VENC_2D_SRC_DST_FMT_BGRA8888 = const(10)
K_VENC_2D_SRC_DST_FMT_BGRA4444 = const(11)
K_VENC_2D_SRC_DST_FMT_BGRA5551 = const(12)
K_VENC_2D_SRC_DST_FMT_BGRX8888 = const(13)
K_VENC_2D_SRC_DST_FMT_BGRX4444 = const(14)
K_VENC_2D_SRC_DST_FMT_BGRX5551 = const(15)
K_VENC_2D_SRC_DST_FMT_RGB888 = const(16)
K_VENC_2D_SRC_DST_FMT_BGR888 = const(17)
K_VENC_2D_SRC_DST_FMT_RGB565 = const(18)
K_VENC_2D_SRC_DST_FMT_BGR565 = const(19)
K_VENC_2D_SRC_DST_FMT_SEPERATE_RGB = const(20)
K_VENC_2D_SRC_DST_FMT_BUTT = const(21)

K_VENC_2D_OSD_FMT_ARGB8888 = const(0)
K_VENC_2D_OSD_FMT_ARGB4444 = const(1)
K_VENC_2D_OSD_FMT_ARGB1555 = const(2)
K_VENC_2D_OSD_FMT_XRGB8888 = const(3)
K_VENC_2D_OSD_FMT_XRGB4444 = const(4)
K_VENC_2D_OSD_FMT_XRGB1555 = const(5)
K_VENC_2D_OSD_FMT_BGRA8888 = const(6)
K_VENC_2D_OSD_FMT_BGRA4444 = const(7)
K_VENC_2D_OSD_FMT_BGRA5551 = const(8)
K_VENC_2D_OSD_FMT_BGRX8888 = const(9)
K_VENC_2D_OSD_FMT_BGRX4444 = const(10)
K_VENC_2D_OSD_FMT_BGRX5551 = const(11)
K_VENC_2D_OSD_FMT_RGB888 = const(12)
K_VENC_2D_OSD_FMT_BGR888 = const(13)
K_VENC_2D_OSD_FMT_RGB565 = const(14)
K_VENC_2D_OSD_FMT_BGR565 = const(15)
K_VENC_2D_OSD_FMT_SEPERATE_RGB = const(16)
K_VENC_2D_OSD_FMT_BUTT = const(17)

K_VENC_2D_ADD_ORDER_VIDEO_OSD = const(0)
K_VENC_2D_ADD_ORDER_OSD_VIDEO = const(1)
K_VENC_2D_ADD_ORDER_VIDEO_BG = const(2)
K_VENC_2D_ADD_ORDER_BG_VIDEO = const(3)
K_VENC_2D_ADD_ORDER_VIDEO_BG_OSD = const(4)
K_VENC_2D_ADD_ORDER_VIDEO_OSD_BG = const(5)
K_VENC_2D_ADD_ORDER_BG_VIDEO_OSD = const(6)
K_VENC_2D_ADD_ORDER_BG_OSD_VIDEO = const(7)
K_VENC_2D_ADD_ORDER_OSD_VIDEO_BG = const(8)
K_VENC_2D_ADD_ORDER_OSD_BG_VIDEO = const(9)
K_VENC_2D_ADD_ORDER_BUTT = const(10)

K_VENC_2D_ATTR_TYPE_BASE = const(0)
K_VENC_2D_ATTR_TYPE_OSD = const(1)
K_VENC_2D_ATTR_TYPE_BORDER = const(2)
K_VENC_2D_ATTR_TYPE_CSC_COEF = const(3)
K_VENC_2D_ATTR_TYPE_OSD_COEF = const(4)
K_VENC_2D_ATTR_TYPE_BUTT = const(5)

VENC_2D_COLOR_GAMUT_BT601 = const(0)
VENC_2D_COLOR_GAMUT_BT709 = const(1)
VENC_2D_COLOR_GAMUT_BT2020 = const(2)
VENC_2D_COLOR_GAMUT_BUTT = const(3)

def k_venc_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_attr_desc, layout)
    venc_def.k_venc_attr_parse(s, kwargs)
    return s

def k_venc_cbr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_cbr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_cbr_desc, layout)
    venc_def.k_venc_cbr_parse(s, kwargs)
    return s

def k_venc_vbr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_vbr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_vbr_desc, layout)
    venc_def.k_venc_vbr_parse(s, kwargs)
    return s

def k_venc_fixqp(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_fixqp_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_fixqp_desc, layout)
    venc_def.k_venc_fixqp_parse(s, kwargs)
    return s

def k_venc_mjpeg_fixqp(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_mjpeg_fixqp_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_mjpeg_fixqp_desc, layout)
    venc_def.k_venc_mjpeg_fixqp_parse(s, kwargs)
    return s

def k_venc_rc_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_rc_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_rc_attr_desc, layout)
    venc_def.k_venc_rc_attr_parse(s, kwargs)
    return s

def k_venc_chn_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_chn_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_chn_attr_desc, layout)
    venc_def.k_venc_chn_attr_parse(s, kwargs)
    return s

def k_venc_pack(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_pack_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_pack_desc, layout)
    venc_def.k_venc_pack_parse(s, kwargs)
    return s

def k_venc_stream(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_stream_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_stream_desc, layout)
    venc_def.k_venc_stream_parse(s, kwargs)
    return s

def k_venc_stream_info(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_stream_info_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_stream_info_desc, layout)
    venc_def.k_venc_stream_info_parse(s, kwargs)
    return s

def k_venc_chn_status(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_chn_status_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_chn_status_desc, layout)
    venc_def.k_venc_chn_status_parse(s, kwargs)
    return s

def k_venc_h265_sao(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_h265_sao_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_h265_sao_desc, layout)
    venc_def.k_venc_h265_sao_parse(s, kwargs)
    return s

def k_venc_rect(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_rect_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_rect_desc, layout)
    venc_def.k_venc_rect_parse(s, kwargs)
    return s

def k_venc_roi_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_roi_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_roi_attr_desc, layout)
    venc_def.k_venc_roi_attr_parse(s, kwargs)
    return s

def k_venc_h264_entropy(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_h264_entropy_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_h264_entropy_desc, layout)
    venc_def.k_venc_h264_entropy_parse(s, kwargs)
    return s

def k_venc_h265_entropy(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_h265_entropy_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_h265_entropy_desc, layout)
    venc_def.k_venc_h265_entropy_parse(s, kwargs)
    return s

def k_venc_2d_csc_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_2d_csc_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_2d_csc_attr_desc, layout)
    venc_def.k_venc_2d_csc_attr_parse(s, kwargs)
    return s

def k_venc_2d_osd_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_2d_osd_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_2d_osd_attr_desc, layout)
    venc_def.k_venc_2d_osd_attr_parse(s, kwargs)
    return s

def k_venc_2d_border_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_2d_border_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_2d_border_attr_desc, layout)
    venc_def.k_venc_2d_border_attr_parse(s, kwargs)
    return s

def k_venc_2d_coef_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(venc_def.k_venc_2d_coef_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), venc_def.k_venc_2d_coef_attr_desc, layout)
    venc_def.k_venc_2d_coef_attr_parse(s, kwargs)
    return s
