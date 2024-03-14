from mpp import *
import _thread


MAX_MEDIA_BUFFER_POOLS = 16 #FIX VALUE

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


class media_device:
    def __init__(self, mod_id, dev_id, chn_id):
        self.mod_id = mod_id
        self.dev_id = dev_id
        self.chn_id = chn_id


class media_buffer:
    def __init__(self, handle, pool_id, phys_addr, virt_addr, size):
        self.handle = handle
        self.pool_id = pool_id
        self.phys_addr = phys_addr
        self.virt_addr = virt_addr
        self.size = size


class media:
    buf_config = k_vb_config()
    buf_config.max_pool_cnt = MAX_MEDIA_BUFFER_POOLS
    config_index = 0
    __buf_has_init = False

    lock = _thread.allocate_lock()

    # create media link
    @classmethod
    def create_link(cls, souce, sink):
        src_mpp = k_mpp_chn()
        dst_mpp = k_mpp_chn()

        src_mpp.mod_id = souce.mod_id
        src_mpp.dev_id = souce.dev_id
        src_mpp.chn_id = souce.chn_id

        dst_mpp.mod_id = sink.mod_id
        dst_mpp.dev_id = sink.dev_id
        dst_mpp.chn_id = sink.chn_id

        ret = kd_mpi_sys_bind(src_mpp, dst_mpp)
        if ret:
            raise OSError(f"create_link failed ret({ret})")


    # destroy media unlink
    @classmethod
    def destroy_link(cls, souce, sink):
        src_mpp = k_mpp_chn()
        dst_mpp = k_mpp_chn()

        src_mpp.mod_id = souce.mod_id
        src_mpp.dev_id = souce.dev_id
        src_mpp.chn_id = souce.chn_id

        dst_mpp.mod_id = sink.mod_id
        dst_mpp.dev_id = sink.dev_id
        dst_mpp.chn_id = sink.chn_id

        ret = kd_mpi_sys_unbind(src_mpp, dst_mpp)
        if ret:
            raise OSError(f"destroy_link failed ret({ret})")


    @classmethod
    def buffer_config(cls, config):
        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if config.comm_pool[i].blk_size and config.comm_pool[i].blk_cnt:
                if cls.config_index > MAX_MEDIA_BUFFER_POOLS - 1:
                    raise OSError("buffer_config failed, pool exceeds the max")

                cls.buf_config.comm_pool[cls.config_index].blk_size = config.comm_pool[i].blk_size
                cls.buf_config.comm_pool[cls.config_index].blk_cnt = config.comm_pool[i].blk_cnt
                cls.buf_config.comm_pool[cls.config_index].mode = config.comm_pool[i].mode
                cls.config_index += 1


    @classmethod
    def buffer_init(cls):
        if cls.__buf_has_init:
            raise OSError("The buffer has been initialized!!!\n\
            This method can only be called once, please check your code!!!")

        # for JPEG encoder
        cls.buf_config.comm_pool[cls.config_index].blk_size = 1843200
        cls.buf_config.comm_pool[cls.config_index].blk_cnt = 16
        cls.buf_config.comm_pool[cls.config_index].mode = VB_REMAP_MODE_NOCACHE
        cls.config_index += 1
        # for VO writeback
        cls.buf_config.comm_pool[cls.config_index].blk_size = 3117056
        cls.buf_config.comm_pool[cls.config_index].blk_cnt = 4
        cls.buf_config.comm_pool[cls.config_index].mode = VB_REMAP_MODE_NOCACHE
        cls.config_index += 1

        print("buffer pool : ", cls.config_index)
        ret = kd_mpi_vb_set_config(cls.buf_config)
        if ret:
            raise OSError(f"buffer_init, vb config failed({ret})")

        supplement_config = k_vb_supplement_config()
        supplement_config.supplement_config |= VB_SUPPLEMENT_JPEG_MASK
        ret = kd_mpi_vb_set_supplement_config(supplement_config)
        if ret:
            raise OSError(f"buffer_init, vb supplement config failed({ret})")

        ret = kd_mpi_vb_init()
        if ret:
            raise OSError(f"buffer_init, vb init failed({ret})")

        cls.__buf_has_init = True
        ide_dbg_vo_wbc_init()


    @classmethod
    def buffer_deinit(cls, force=False):
        cls.buf_config.max_pool_cnt = 0
        cls.config_index = 0
        cls.__buf_has_init = False
        if not force:
            return
        ret = kd_mpi_vb_exit()
        if ret:
            raise OSError(f"buffer_deinit, vb deinit failed({ret})")


    @classmethod
    def request_buffer(cls, size):

        buf_handle = kd_mpi_vb_get_block(VB_INVALID_POOLID, size, '')
        if buf_handle == VB_INVALID_HANDLE:
            raise OSError(f"request_buffer, get buf block failed, size {size}")

        pool_id = kd_mpi_vb_handle_to_pool_id(buf_handle)
        if pool_id == VB_INVALID_POOLID:
            kd_mpi_vb_release_block(buf_handle)
            raise OSError("request_buffer, get buf pool id error")

        phys_addr = kd_mpi_vb_handle_to_phyaddr(buf_handle)
        if phys_addr == 0:
            kd_mpi_vb_release_block(buf_handle)
            raise OSError("request_buffer, get buf phys addr failed")

        virt_addr = kd_mpi_sys_mmap(phys_addr, size)
        if virt_addr == 0:
            kd_mpi_vb_release_block(buf_handle)
            raise OSError("request_buffer, map buf virt addr failed")

        buffer = media_buffer(buf_handle, pool_id, phys_addr, virt_addr, size)
        return buffer


    @classmethod
    def release_buffer(cls, buffer):
        ret = kd_mpi_vb_release_block(buffer.handle)
        if ret:
            raise OSError("release_buffer, release buf block failed")

        ret = kd_mpi_sys_munmap(buffer.virt_addr, buffer.size)
        if ret:
            raise OSError("release_buffer, munmap failed")

