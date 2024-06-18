import uctypes
from mpp import vo_def

k_connectori_phy_attr_desc = {
    "n": 0 | uctypes.UINT32,
    "m": 4 | uctypes.UINT32,
    "voc": 8 | uctypes.UINT32,
    "hs_freq": 12 | uctypes.UINT32,
}

def k_connectori_phy_attr_parse(s, kwargs):
    s.n = kwargs.get("n", 0)
    s.m = kwargs.get("m", 0)
    s.voc = kwargs.get("voc", 0)
    s.hs_freq = kwargs.get("hs_freq", 0)

k_connector_info_desc = {
    "connector_name": 0 | uctypes.UINT64,
    "screen_test_mode": 8 | uctypes.UINT32,
    "dsi_test_mode": 12 | uctypes.UINT32,
    "bg_color": 16 | uctypes.UINT32,
    "intr_line": 20 | uctypes.UINT32,
    "pixclk_div": 24 | uctypes.UINT32,
    "lan_num": 28 | uctypes.UINT32,
    "work_mode": 32 | uctypes.UINT32,
    "cmd_mode": 36 | uctypes.UINT32,
    "phy_attr": (52, k_connectori_phy_attr_desc),
    "resolution": (56, vo_def.k_vo_display_resolution_desc),
    "type": 108 | uctypes.UINT32,
}

def k_connector_info_parse(s, kwargs):
    s.connector_name = kwargs.get("connector_name", 0)
    s.screen_test_mode = kwargs.get("screen_test_mode", 0)
    s.dsi_test_mode = kwargs.get("dsi_test_mode", 0)
    s.bg_color = kwargs.get("bg_color", 0)
    s.intr_line = kwargs.get("intr_line", 0)
    s.lan_num = kwargs.get("lan_num", 0)
    s.work_mode = kwargs.get("work_mode", 0)
    s.cmd_mode = kwargs.get("cmd_mode", 0)
    phy_attr = kwargs.get("phy_attr", {})
    k_connectori_phy_attr_parse(s.phy_attr, phy_attr)
    resolution = kwargs.get("resolution", {})
    vo_def.k_vo_display_resolution_parse(s.resolution, resolution)
    s.type = kwargs.get("type", 0)
