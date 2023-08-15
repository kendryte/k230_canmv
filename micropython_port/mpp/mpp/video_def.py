import uctypes

k_video_supplement_desc = {
    "jpeg_dcf_phy_addr": 0 | uctypes.UINT64,
    "isp_info_phy_addr": 8 | uctypes.UINT64,
    "jpeg_dcf_kvirt_addr": 16 | uctypes.UINT64,
    "isp_info_kvirt_addr": 24 | uctypes.UINT64,
}

def k_video_supplement_parse(s, kwargs):
    s.jpeg_dcf_phy_addr = kwargs.get("jpeg_dcf_phy_addr", 0)
    s.isp_info_phy_addr = kwargs.get("isp_info_phy_addr", 0)
    s.jpeg_dcf_kvirt_addr = kwargs.get("jpeg_dcf_kvirt_addr", 0)
    s.isp_info_kvirt_addr = kwargs.get("isp_info_kvirt_addr", 0)

k_video_frame_desc = {
    "width": 0 | uctypes.UINT32,
    "height": 4 | uctypes.UINT32,
    "field": 8 | uctypes.UINT32,
    "pixel_format": 12 | uctypes.UINT32,
    "video_format": 16 | uctypes.UINT32,
    "dynamic_range": 20 | uctypes.UINT32,
    "compress_mode": 24 | uctypes.UINT32,
    "color_gamut": 28 | uctypes.UINT32,
    "header_stride": (32 | uctypes.ARRAY, 3 | uctypes.UINT32),
    "stride": (44 | uctypes.ARRAY, 3 | uctypes.UINT32),
    "header_phys_addr": (56 | uctypes.ARRAY, 3 | uctypes.UINT64),
    "header_virt_addr": (80 | uctypes.ARRAY, 3 | uctypes.UINT64),
    "phys_addr": (104 | uctypes.ARRAY, 3 | uctypes.UINT64),
    "virt_addr": (128 | uctypes.ARRAY, 3 | uctypes.UINT64),
    "offset_top": 152 | uctypes.INT16,
    "offset_bottom": 154 | uctypes.INT16,
    "offset_left": 156 | uctypes.INT16,
    "offset_right": 158 | uctypes.INT16,
    "time_ref": 160 | uctypes.UINT32,
    "pts": 168 | uctypes.UINT64,
    "priv_data": 176 | uctypes.UINT64,
    "supplement": (184, k_video_supplement_desc),
}

def k_video_frame_parse(s, kwargs):
    s.width = kwargs.get("width", 0)
    s.height = kwargs.get("height", 0)
    s.field = kwargs.get("field", 0)
    s.pixel_format = kwargs.get("pixel_format", 0)
    s.video_format = kwargs.get("video_format", 0)
    s.dynamic_range = kwargs.get("dynamic_range", 0)
    s.compress_mode = kwargs.get("compress_mode", 0)
    s.color_gamut = kwargs.get("color_gamut", 0)
    s.offset_top = kwargs.get("offset_top", 0)
    s.offset_bottom = kwargs.get("offset_bottom", 0)
    s.offset_left = kwargs.get("offset_left", 0)
    s.offset_right = kwargs.get("offset_right", 0)
    s.time_ref = kwargs.get("time_ref", 0)
    s.pts = kwargs.get("pts", 0)
    s.priv_data = kwargs.get("priv_data", 0)
    supplement = kwargs.get("supplement", {})
    k_video_supplement_parse(s, supplement)

k_video_frame_info_desc = {
    "v_frame": (0, k_video_frame_desc),
    "pool_id": 216 | uctypes.UINT32,
    "mod_id": 220 | uctypes.UINT32,
}

def k_video_frame_info_parse(s, kwargs):
    v_frame = kwargs.get("v_frame", {})
    s.pool_id = kwargs.get("pool_id", 0)
    s.mod_id = kwargs.get("mod_id", 0)
