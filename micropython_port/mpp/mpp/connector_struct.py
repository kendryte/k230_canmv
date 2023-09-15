import uctypes
from mpp import connector_def

HX8377_V2_MIPI_4LAN_1080X1920_30FPS = const(0)
LT9611_MIPI_4LAN_1920X1080_30FPS = const(1)

def k_connector_info(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(connector_def.k_connector_info_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), connector_def.k_connector_info_desc, layout)
    connector_def.k_connector_info_parse(s, kwargs)
    return s

def k_connector_info(**kwargs):
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(connector_def.k_connector_info_desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), connector_def.k_connector_info_desc, layout)
    connector_def.k_connector_info_parse(s, kwargs)
    return s
