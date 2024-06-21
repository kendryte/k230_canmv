from mpp import *

import time

MAX_MEDIA_BUFFER_POOLS = 16 #FIX VALUE

MAX_MEDIA_LINK_COUNT = 8

# K230 CanMV meida module define
AUDIO_IN_MOD_ID = K_ID_AI          # audio in device module
AUDIO_OUT_MOD_ID = K_ID_AO         # audio out device module

AUDIO_ENCODE_MOD_ID = K_ID_AENC    # audio encode device module
AUDIO_DECODE_MOD_ID = K_ID_ADEC    # audio decode device module

CAMERA_MOD_ID = K_ID_VI            # camera cdevice module

DISPLAY_MOD_ID = K_ID_VO            # display device module

DMA_MOD_ID = K_ID_DMA              # DMA device module

DPU_MOD_ID = K_ID_DPU              # DPU device module

VIDEO_ENCODE_MOD_ID = K_ID_VENC    # video encode device module
VIDEO_DECODE_MOD_ID = K_ID_VDEC    # video decode device module


# audio device id definition
# TODO

# camera device id definition
CAM_DEV_ID_0 = VICAP_DEV_ID_0
CAM_DEV_ID_1 = VICAP_DEV_ID_1
CAM_DEV_ID_2 = VICAP_DEV_ID_2
CAM_DEV_ID_MAX = VICAP_DEV_ID_MAX

# display device id definition
DISPLAY_DEV_ID = K_VO_DISPLAY_DEV_ID

# DMA device id definition
# TODO

# DPU device id definition
# TODO

# video encode device id definition
VENC_DEV_ID = const(0)

# video decode device id definition
# TODO
VDEC_DEV_ID = const(0)

# audio channel id definition
# TODO

# camera channel id definition
CAM_CHN_ID_0 = VICAP_CHN_ID_0
CAM_CHN_ID_1 = VICAP_CHN_ID_1
CAM_CHN_ID_2 = VICAP_CHN_ID_2
CAM_CHN_ID_MAX = VICAP_CHN_ID_MAX

# display channel id definition
DISPLAY_CHN_ID_0 = K_VO_DISPLAY_CHN_ID0
DISPLAY_CHN_ID_1 = K_VO_DISPLAY_CHN_ID1
DISPLAY_CHN_ID_2 = K_VO_DISPLAY_CHN_ID2
DISPLAY_CHN_ID_3 = K_VO_DISPLAY_CHN_ID3
DISPLAY_CHN_ID_4 = K_VO_DISPLAY_CHN_ID4
DISPLAY_CHN_ID_5 = K_VO_DISPLAY_CHN_ID5
DISPLAY_CHN_ID_6 = K_VO_DISPLAY_CHN_ID6

# DMA channel id definition
# TODO

# DPU channel id definition
# TODO

# video encode channel id definition
VENC_CHN_ID_0 = const(0)
VENC_CHN_ID_1 = const(1)
VENC_CHN_ID_2 = const(2)
VENC_CHN_ID_3 = const(3)
VENC_CHN_ID_MAX = VENC_MAX_CHN_NUMS
VENC_PACK_CNT_MAX = const(12)

# video decode channel id definition
VDEC_CHN_ID_0 = const(0)
VDEC_CHN_ID_1 = const(1)
VDEC_CHN_ID_2 = const(2)
VDEC_CHN_ID_3 = const(3)
VDEC_CHN_ID_MAX = VDEC_MAX_CHN_NUMS


# data align up
ALIGN_UP = VICAP_ALIGN_UP

VB_INVALID_POOLID = 0xFFFFFFFF
VB_INVALID_HANDLE = 0xFFFFFFFF

class MediaManager:

    class Buffer:
        def __init__(self, handle, pool_id, phys_addr, virt_addr, size, manged = False):
            self.handle = handle
            self.pool_id = pool_id
            self.phys_addr = phys_addr
            self.virt_addr = virt_addr
            self.size = size
            self.manged = manged

        def __del__(self):
            if self.manged:
                MediaManager._del_buffer(self)

            info = vb_block_info()
            info.size = self.size
            info.handle = self.handle
            info.virt_addr = self.virt_addr

            ret = vb_mgmt_put_block(info)
            if ret != 0:
                raise RuntimeError(f"MediaManager.Buffer, release buf block failed({ret})")

        def __eq__(self, other):
            if isinstance(other, MediaManager.Buffer):
                return self.handle == other.handle and self.virt_addr == other.virt_addr and self.size == other.size
            return False

        def __str__(self):
            return f"MediaManager.Buffer: handle {self.handle}, size {self.size}, poolId {self.pool_id}, phyAddr {self.phys_addr}, virtAddr {self.virt_addr:x}"

        @staticmethod
        def get(size):
            if not MediaManager._is_inited:
                raise AssertionError("please run MediaManager.Buffer() after MediaManager.init()")

            info = vb_block_info()
            info.size = size

            ret = vb_mgmt_get_block(info)
            if ret != 0:
                raise RuntimeError(f"MediaManager.Buffer, get buf block failed, size {size}, ret({ret})")

            buffer = MediaManager.Buffer(info.handle, info.pool_id, info.phys_addr, info.virt_addr, info.size, manged = True)

            if not MediaManager._add_buffer(buffer):
                vb_mgmt_put_block(info)
                buffer.__del__()
                buffer = None
                raise RuntimeError("MediaManager.Buffer, add buffer to manager failed")

            return buffer

    _vb_buffer_index = 0
    _vb_buffer = k_vb_config()
    _vb_buffer.max_pool_cnt = MAX_MEDIA_BUFFER_POOLS

    _buffers = [None for i in range(0, MAX_MEDIA_BUFFER_POOLS)]
    _links = [None for i in range(0, MAX_MEDIA_LINK_COUNT)]
    _is_inited = False

    @classmethod
    def init(cls):
        if cls._is_inited:
            raise AssertionError("The buffer has been initialized!!!\n\
            This method can only be called once, please check your code!!!")

        print("buffer pool : ", cls._vb_buffer_index)
        ret = kd_mpi_vb_set_config(cls._vb_buffer)
        if ret:
            raise RuntimeError(f"MediaManager, vb config failed({ret})")

        supplement_config = k_vb_supplement_config()
        supplement_config.supplement_config |= VB_SUPPLEMENT_JPEG_MASK
        ret = kd_mpi_vb_set_supplement_config(supplement_config)
        if ret:
            raise RuntimeError(f"MediaManager, vb supplement config failed({ret})")

        ret = kd_mpi_vb_init()
        if ret:
            raise RuntimeError(f"MediaManager, vb init failed({ret})")

        ide_dbg_vo_wbc_init()
        cls._is_inited = True

    @classmethod
    def deinit(cls, force = False):
        time.sleep_ms(100)

        cls._vb_buffer.max_pool_cnt = MAX_MEDIA_BUFFER_POOLS
        cls._vb_buffer_index = 0
        cls._is_inited = False

        cls._destory_all_link()
        cls._destory_all_buffer()

        if not force:
            return

        ret = kd_mpi_vb_exit()
        if ret:
            raise RuntimeError(f"MediaManager, vb deinit failed({ret})")

    @classmethod
    def _config(cls, config):
        if MediaManager._is_inited:
            raise AssertionError("please run MediaManager._config() before MediaManager.init()")

        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if config.comm_pool[i].blk_size and config.comm_pool[i].blk_cnt:
                if cls._vb_buffer_index > MAX_MEDIA_BUFFER_POOLS - 1:
                    print("MediaManager buffer configure failed, pool exceeds the max")
                    return False

                cls._vb_buffer.comm_pool[cls._vb_buffer_index].blk_size = config.comm_pool[i].blk_size
                cls._vb_buffer.comm_pool[cls._vb_buffer_index].blk_cnt = config.comm_pool[i].blk_cnt
                cls._vb_buffer.comm_pool[cls._vb_buffer_index].mode = config.comm_pool[i].mode
                cls._vb_buffer_index += 1

        return True

    class linker:
        def __init__(self, src, dst):
            self.src = src
            self.dst = dst

        def __del__(self):
            MediaManager._del_link(self)

        def __eq__(self, o):
            if isinstance(o, MediaManager.linker):
                if self.src.mod_id != o.src.mod_id:
                    return False
                if self.src.dev_id != o.src.dev_id:
                    return False
                if self.src.chn_id != o.src.chn_id:
                    return False
                if self.dst.mod_id != o.dst.mod_id:
                    return False
                if self.dst.dev_id != o.dst.dev_id:
                    return False
                if self.dst.chn_id != o.dst.chn_id:
                    return False
                return True
            return False

        def __str__(self):
            return f"linker src {self.src.mod_id}:{self.src.dev_id}:{self.src.chn_id}, dst {self.dst.mod_id}:{self.dst.dev_id}:{self.dst.chn_id}"

    @classmethod
    def link(cls, src, dst):
        if MediaManager._is_inited:
            raise AssertionError("please run MediaManager._link() before MediaManager.init()")

        if not isinstance(src, tuple):
            raise TypeError("src is not a tuple")
        if len(src) != 3:
            raise TypeError("src size shoule be 3")

        src_mpp = k_mpp_chn()
        src_mpp.mod_id = src[0]
        src_mpp.dev_id = src[1]
        src_mpp.chn_id = src[2]

        if not isinstance(dst, tuple):
            raise TypeError("dst is not a tuple")
        if len(dst) != 3:
            raise TypeError("dst size shoule be 3")

        dst_mpp = k_mpp_chn()
        dst_mpp.mod_id = dst[0]
        dst_mpp.dev_id = dst[1]
        dst_mpp.chn_id = dst[2]

        ret = kd_mpi_sys_bind(src_mpp, dst_mpp)
        if ret:
            raise RuntimeError(f"MediaManager link {src} to {dst} failed({ret})")

        l = MediaManager.linker(src_mpp, dst_mpp)
        if not cls._add_link(l):
            kd_mpi_sys_unbind(src_mpp, dst_mpp)
            raise RuntimeError(f"MediaManager link {src} to {dst} failed(add to links)")

        vb_mgmt_push_link_info(src_mpp, dst_mpp)

        return l

    @classmethod
    def _add_buffer(cls, buffer):
        if not isinstance(buffer, MediaManager.Buffer):
            raise TypeError("buffer is not MediaManager.Buffer")
        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if cls._buffers[i] is None:
                cls._buffers[i] = buffer
                return True
        return False

    @classmethod
    def _del_buffer(cls, buffer):
        if not isinstance(buffer, MediaManager.Buffer):
            raise TypeError("buffer is not MediaManager.Buffer")
        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if cls._buffers[i] == buffer:
                cls._buffers[i] = None
                return
        print(f"del buffer failed {buffer}") 

    @classmethod
    def _destory_all_buffer(cls):
        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if isinstance(cls._buffers[i], MediaManager.Buffer):
                cls._buffers[i].__del__()

    @classmethod
    def _add_link(cls, link):
        if not isinstance(link, MediaManager.linker):
            raise TypeError("link is not MediaManager.linker")
        for i in range(0, MAX_MEDIA_LINK_COUNT):
            if cls._links[i] is None:
                cls._links[i] = link
                return True
        return False

    @classmethod
    def _del_link(cls, link):
        if not isinstance(link, MediaManager.linker):
            raise TypeError("link is not MediaManager.linker")
        for i in range(0, MAX_MEDIA_LINK_COUNT):
            if cls._links[i] == link:
                # ret = kd_mpi_sys_unbind(link.src, link.dst)
                ret = vb_mgmt_pop_link_info(link.src, link.dst)
                if ret:
                    print(f"_del_link failed ret({ret})")
                cls._links[i] = None
                return
        print(f"del link failed {link}") 

    @classmethod
    def _destory_all_link(cls):
        for i in range(0, MAX_MEDIA_LINK_COUNT):
            if isinstance(cls._links[i], MediaManager.linker):
                cls._del_link(cls._links[i])
