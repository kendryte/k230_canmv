import uctypes

k_mpp_chn_desc = {
    "mod_id": 0 | uctypes.UINT32,
    "dev_id": 4 | uctypes.UINT32,
    "chn_id": 8 | uctypes.UINT32,
}

def k_mpp_chn_parse(s, kwargs):
    s.mod_id = kwargs.get("mod_id", 0)
    s.dev_id = kwargs.get("dev_id", 0)
    s.chn_id = kwargs.get("chn_id", 0)

k_mpp_bind_dest_desc = {
    "num": 0 | uctypes.UINT32,
    "mpp_chn": (4 | uctypes.ARRAY, 64, k_mpp_chn_desc),
}

def k_mpp_bind_dest_parse(s, kwargs):
    s.num = kwargs.get("num", 0)

k_log_level_conf_desc = {
    "mod_id": 0 | uctypes.UINT32,
    "level": 4 | uctypes.UINT32,
    "mod_name": (8 | uctypes.ARRAY, 16 | uctypes.UINT8),
}

def k_log_level_conf_parse(s, kwargs):
    s.mod_id = kwargs.get("mod_id", 0)
    s.level = kwargs.get("level", 0)

k_sys_virmem_info_desc = {
    "phy_addr": 0 | uctypes.UINT64,
    "cached": 8 | uctypes.UINT32,
}

def k_sys_virmem_info_parse(s, kwargs):
    s.phy_addr = kwargs.get("phy_addr", 0)
    s.cached = kwargs.get("cached", 0)
