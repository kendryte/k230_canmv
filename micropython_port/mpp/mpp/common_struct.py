import uctypes

def _k_ptr_base(type, *args):
    desc = {"value": type}
    layout = uctypes.NATIVE
    buf = bytearray(uctypes.sizeof(desc, layout))
    s = uctypes.struct(uctypes.addressof(buf), desc, layout)
    s.value = 0 if len(args) == 0 else int(args[0])
    return s

def k_u64_ptr(*args):
    return _k_ptr_base(uctypes.UINT64, args)

def k_u32_ptr(*args):
    return _k_ptr_base(uctypes.UINT32, args)

def k_u8_ptr(*args):
    return _k_ptr_base(uctypes.UINT8, args)

def k_bool_ptr(*args):
    return _k_ptr_base(uctypes.UINT32, args)

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
