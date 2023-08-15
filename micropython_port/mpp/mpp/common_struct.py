import uctypes

def _k_ptr_base(type, value=0):
    desc = {"value": type}
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), desc, layout)
    s.value = value
    return s

def k_u64_ptr(value=0):
    return _k_ptr_base(uctypes.UINT64, value)

def k_u32_ptr(value=0):
    return _k_ptr_base(uctypes.UINT32, value)

def k_u8_ptr(value=0):
    return _k_ptr_base(uctypes.UINT8, value)

def k_bool_ptr(value=0):
    return _k_ptr_base(uctypes.UINT32, value)

def struct_ptr(s):
    return uctypes.addressof(s)

def str_ptr(s):
    return uctypes.addressof(s)

def struct_copy(src, dst):
    if (type(src) != uctypes.struct or type(dst) != uctypes.struct):
        print("src or dst type is not uctypes.struct")
        return None
    src_size = uctypes.sizeof(src)
    dst_size = uctypes.sizeof(dst)
    if (src_size != dst_size):
        print("src size not equal dst size")
        return None
    uctypes.bytearray_at(uctypes.addressof(dst), dst_size)[:] = uctypes.bytearray_at(uctypes.addressof(src), src_size)
