import uctypes
from mpp import vo_def

K_VO_LAYER0 = const(0)
K_VO_LAYER1 = const(1)
K_VO_LAYER2 = const(2)

K_VO_OSD0 = const(0)
K_VO_OSD1 = const(1)
K_VO_OSD2 = const(2)
K_VO_OSD3 = const(3)

K_VO_BIND_CHANGE_POSTION = const(1)
K_VO_ONLY_CHANGE_PHYADDR = const(2)
K_VO_CHANGE_POSTION = const(3)

K_ADDR_MODE_ONLY_PING = const(0)
K_ADDR_MODE_ONLY_PANG = const(1)
K_ADDR_MODE_PANG_PANG = const(2)

K_VO_LAYER_Y_ENDIAN_DODE0 = const(0)
K_VO_LAYER_Y_ENDIAN_DODE1 = const(1)
K_VO_LAYER_Y_ENDIAN_DODE2 = const(2)
K_VO_LAYER_Y_ENDIAN_DODE3 = const(3)

K_VO_LAYER_UV_ENDIAN_DODE0 = const(0)
K_VO_LAYER_UV_ENDIAN_DODE1 = const(1)
K_VO_LAYER_UV_ENDIAN_DODE2 = const(2)
K_VO_LAYER_UV_ENDIAN_DODE3 = const(3)
K_VO_LAYER_UV_ENDIAN_DODE4 = const(4)
K_VO_LAYER_UV_ENDIAN_DODE5 = const(5)
K_VO_LAYER_UV_ENDIAN_DODE6 = const(6)
K_VO_LAYER_UV_ENDIAN_DODE7 = const(7)

K_VO_INTF_MIPI = const(0)

K_VO_OUT_1080P30 = const(0)
K_VO_OUT_1080P60 = const(1)

K_VO_LP_MODE = const(0)
K_VO_HS_MODE = const(1)

K_VO_OSD_MAP_ORDER = const(0)
K_VO_OSD_MAP_1234_TO_2143 = const(2)
K_VO_OSD_MAP_1234_TO_4321 = const(3)

K_DSI_1LAN = const(0)
K_DSI_2LAN = const(1)
K_DSI_4LAN = const(3)

K_BURST_MODE = const(0)
K_NO_BURST_MODE = const(1)

K_ROTATION_0 = const((1 << 0))
K_ROTATION_90 = const((1 << 1))
K_ROTATION_180 = const((1 << 2))
K_ROTATION_270 = const((1 << 3))

K_VO_MIRROR_NONE = const((1 << 4))
K_VO_MIRROR_HOR = const((1 << 5))
K_VO_MIRROR_VER = const((1 << 6))
K_VO_MIRROR_BOTH = const((1 << 7))

K_VO_GRAY_ENABLE = const((1 << 8))
K_VO_GRAY_DISABLE = const((1 << 9))

K_VO_SCALER_ENABLE = const((1 << 10))
K_VO_SCALER_DISABLE = const((1 << 11))

def k_vo_point(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_point_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_point_desc, layout)
    vo_def.k_vo_point_parse(s, kwargs)
    return s

def k_vo_size(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_size_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_size_desc, layout)
    vo_def.k_vo_size_parse(s, kwargs)
    return s

def k_vo_scaler_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_scaler_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_scaler_attr_desc, layout)
    vo_def.k_vo_scaler_attr_parse(s, kwargs)
    return s

def k_vo_display_resolution(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_display_resolution_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_display_resolution_desc, layout)
    vo_def.k_vo_display_resolution_parse(s, kwargs)
    return s

def k_vo_mipi_phy_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_mipi_phy_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_mipi_phy_attr_desc, layout)
    vo_def.k_vo_mipi_phy_attr_parse(s, kwargs)
    return s

def k_vo_pub_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_pub_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_pub_attr_desc, layout)
    vo_def.k_vo_pub_attr_parse(s, kwargs)
    return s

def k_vo_user_sync_info(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_user_sync_info_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_user_sync_info_desc, layout)
    vo_def.k_vo_user_sync_info_parse(s, kwargs)
    return s

def k_vo_wbc_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_wbc_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_wbc_attr_desc, layout)
    vo_def.k_vo_wbc_attr_parse(s, kwargs)
    return s

def k_vo_dsi_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_dsi_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_dsi_attr_desc, layout)
    vo_def.k_vo_dsi_attr_parse(s, kwargs)
    return s

def k_vo_wbc_frame_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_wbc_frame_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_wbc_frame_attr_desc, layout)
    vo_def.k_vo_wbc_frame_attr_parse(s, kwargs)
    return s

def k_vo_video_layer_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_video_layer_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_video_layer_attr_desc, layout)
    vo_def.k_vo_video_layer_attr_parse(s, kwargs)
    return s

def k_vo_video_osd_attr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_video_osd_attr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_video_osd_attr_desc, layout)
    vo_def.k_vo_video_osd_attr_parse(s, kwargs)
    return s

def k_vo_draw_frame(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_draw_frame_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_draw_frame_desc, layout)
    vo_def.k_vo_draw_frame_parse(s, kwargs)
    return s

def k_vo_video_layer_attr_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_video_layer_attr_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_video_layer_attr_p_desc, layout)
    vo_def.k_vo_video_layer_attr_p_parse(s, kwargs)
    return s

def k_vo_mirror_mode_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_mirror_mode_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_mirror_mode_p_desc, layout)
    vo_def.k_vo_mirror_mode_p_parse(s, kwargs)
    return s

def k_vo_rotation_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_rotation_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_rotation_p_desc, layout)
    vo_def.k_vo_rotation_p_parse(s, kwargs)
    return s

def k_vo_gray_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_gray_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_gray_p_desc, layout)
    vo_def.k_vo_gray_p_parse(s, kwargs)
    return s

def k_vo_scaler_attr_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_scaler_attr_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_scaler_attr_p_desc, layout)
    vo_def.k_vo_scaler_attr_p_parse(s, kwargs)
    return s

def k_vo_priority_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_priority_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_priority_p_desc, layout)
    vo_def.k_vo_priority_p_parse(s, kwargs)
    return s

def k_vo_layer_pos_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_layer_pos_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_layer_pos_p_desc, layout)
    vo_def.k_vo_layer_pos_p_parse(s, kwargs)
    return s

def k_vo_osd_alpha_grade_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_alpha_grade_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_alpha_grade_p_desc, layout)
    vo_def.k_vo_osd_alpha_grade_p_parse(s, kwargs)
    return s

def k_vo_osd_pix_format_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_pix_format_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_pix_format_p_desc, layout)
    vo_def.k_vo_osd_pix_format_p_parse(s, kwargs)
    return s

def k_vo_osd_stride_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_stride_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_stride_p_desc, layout)
    vo_def.k_vo_osd_stride_p_parse(s, kwargs)
    return s

def k_vo_osd_attr_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_attr_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_attr_p_desc, layout)
    vo_def.k_vo_osd_attr_p_parse(s, kwargs)
    return s

def k_vo_osd_rb_swap_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_rb_swap_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_rb_swap_p_desc, layout)
    vo_def.k_vo_osd_rb_swap_p_parse(s, kwargs)
    return s

def k_vo_osd_dma_map_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_osd_dma_map_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_osd_dma_map_p_desc, layout)
    vo_def.k_vo_osd_dma_map_p_parse(s, kwargs)
    return s

def k_vo_dsi_send_cmd_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_dsi_send_cmd_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_dsi_send_cmd_p_desc, layout)
    vo_def.k_vo_dsi_send_cmd_p_parse(s, kwargs)
    return s

def k_vo_dsi_read_cmd_p(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_dsi_read_cmd_p_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_dsi_read_cmd_p_desc, layout)
    vo_def.k_vo_dsi_read_cmd_p_parse(s, kwargs)
    return s

def k_vo_phyaddr(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(vo_def.k_vo_phyaddr_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), vo_def.k_vo_phyaddr_desc, layout)
    vo_def.k_vo_phyaddr_parse(s, kwargs)
    return s
