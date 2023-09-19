import uctypes

k_vb_pool_config_desc = {
    "blk_size": 0 | uctypes.UINT64,
    "blk_cnt": 8 | uctypes.UINT32,
    "mode": 12 | uctypes.UINT32,
    "mmz_name": (16 | uctypes.ARRAY, 16 | uctypes.UINT8),
}

def k_vb_pool_config_parse(s, kwargs):
    s.blk_size = kwargs.get("blk_size", 0)
    s.blk_cnt = kwargs.get("blk_cnt", 0)
    s.mode = kwargs.get("mode", 0)

k_vb_config_desc = {
    "max_pool_cnt": 0 | uctypes.UINT32,
    "comm_pool": (8 | uctypes.ARRAY, 16, k_vb_pool_config_desc),
}

def k_vb_config_parse(s, kwargs):
    s.max_pool_cnt = kwargs.get("max_pool_cnt", 0)

k_vb_mod_config_desc = {
    "uid": 0 | uctypes.UINT32,
    "mod_config": (8, k_vb_config_desc),
}

def k_vb_mod_config_parse(s, kwargs):
    s.uid = kwargs.get("uid", 0)

k_vb_pool_info_desc = {
    "pool_id": 0 | uctypes.UINT32,
    "blk_cnt": 4 | uctypes.UINT32,
    "blk_size": 8 | uctypes.UINT64,
    "pool_size": 16 | uctypes.UINT64,
    "pool_phys_addr": 24 | uctypes.UINT64,
    "remap_mode": 32 | uctypes.UINT32,
    "pool_kvirt_addr": 40 | uctypes.UINT64,
    "mmz_name": (48 | uctypes.ARRAY, 16 | uctypes.UINT8),
}

def k_vb_pool_info_parse(s, kwargs):
    s.pool_id = kwargs.get("pool_id", 0)
    s.blk_cnt = kwargs.get("blk_cnt", 0)
    s.blk_size = kwargs.get("blk_size", 0)
    s.pool_size = kwargs.get("pool_size", 0)
    s.pool_phys_addr = kwargs.get("pool_phys_addr", 0)
    s.remap_mode = kwargs.get("remap_mode", 0)
    s.pool_kvirt_addr = kwargs.get("pool_kvirt_addr", 0)

k_vb_supplement_config_desc = {
    "supplement_config": 0 | uctypes.UINT32,
}

def k_vb_supplement_config_parse(s, kwargs):
    s.supplement_config = kwargs.get("supplement_config", 0)

k_vb_pool_status_desc = {
    "is_comm_pool": 0 | uctypes.UINT32,
    "blk_cnt": 4 | uctypes.UINT32,
    "free_blk_cnt": 8 | uctypes.UINT32,
}

def k_vb_pool_status_parse(s, kwargs):
    s.is_comm_pool = kwargs.get("is_comm_pool", 0)
    s.blk_cnt = kwargs.get("blk_cnt", 0)
    s.free_blk_cnt = kwargs.get("free_blk_cnt", 0)
