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

# audio channel id definition


# camera device id definition
CAM_DEV_ID_0 = VICAP_DEV_ID_0
CAM_DEV_ID_1 = VICAP_DEV_ID_1
CAM_DEV_ID_2 = VICAP_DEV_ID_2
CAM_DEV_ID_MAX = VICAP_DEV_ID_MAX

# camera channel id definition
CAM_CHN_ID_0 = VICAP_CHN_ID_0
CAM_CHN_ID_1 = VICAP_CHN_ID_1
CAM_CHN_ID_2 = VICAP_CHN_ID_2
CAM_CHN_ID_MAX = VICAP_CHN_ID_MAX

CAM_ALIGN_UP = VICAP_ALIGN_UP

# display device id definition
DISPLAY_DEV_ID = K_VO_DISPLAY_DEV_ID


# display channel id definition
DISPLAY_CHN_ID_0 = K_VO_DISPLAY_CHN_ID0
DISPLAY_CHN_ID_1 = K_VO_DISPLAY_CHN_ID1
DISPLAY_CHN_ID_2 = K_VO_DISPLAY_CHN_ID2
DISPLAY_CHN_ID_3 = K_VO_DISPLAY_CHN_ID3
DISPLAY_CHN_ID_4 = K_VO_DISPLAY_CHN_ID4
DISPLAY_CHN_ID_5 = K_VO_DISPLAY_CHN_ID5
DISPLAY_CHN_ID_6 = K_VO_DISPLAY_CHN_ID6


# video device id definition

# video channel id definition



class media_device:
    def __init__(self, mod_id, dev_id, chn_id):
        self.mod_id = mod_id
        self.dev_id = dev_id
        self.chn_id = chn_id


class media:
    buf_config = k_vb_config()
    config_index = 0
    __buf_has_init = False

    lock = _thread.allocate_lock()

    # set media link
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
            print(f"create_link failed ret({ret})")
            return ret

        return 0


    # set media unlink
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
            print(f"destroy_link failed ret({ret})")
            return ret

        return 0

    @classmethod
    def buffer_config(cls, config):
        cls.lock.acquire()
        if config.max_pool_cnt > cls.buf_config.max_pool_cnt:
            cls.buf_config.max_pool_cnt = config.max_pool_cnt

        for i in range(0, MAX_MEDIA_BUFFER_POOLS):
            if config.comm_pool[i].blk_size and config.comm_pool[i].blk_cnt:
                if cls.config_index > MAX_MEDIA_BUFFER_POOLS - 1:
                    print("buffer_config failed, pool exceeds the max")
                    cls.lock.release()
                    return -1

                cls.buf_config.comm_pool[cls.config_index].blk_size = config.comm_pool[i].blk_size
                cls.buf_config.comm_pool[cls.config_index].blk_cnt = config.comm_pool[i].blk_cnt
                cls.buf_config.comm_pool[cls.config_index].mode = config.comm_pool[i].mode
                cls.config_index += 1

        cls.lock.release()
        return 0


    @classmethod
    def buffer_init(cls):
        if cls.__buf_has_init:
            print("The buffer has been initialized!!!")
            print("This method can only be called once, please check your code!!!")
            return -1

        ret = kd_mpi_vb_set_config(cls.buf_config)
        if ret:
            print(f"buffer_init, vb config failed({ret})")
            return ret

        supplement_config = k_vb_supplement_config()
        supplement_config.supplement_config |= VB_SUPPLEMENT_JPEG_MASK
        ret = kd_mpi_vb_set_supplement_config(supplement_config)
        if ret:
            print(f"buffer_init, vb supplement config failed({ret})")
            return ret

        ret = kd_mpi_vb_init()
        if ret:
            print(f"buffer_init, vb init failed({ret})")
            return ret
        cls.__buf_has_init = True

        return 0

    @classmethod
    def buffer_deinit(cls):
        cls.buf_config.max_pool_cnt = 0
        cls.config_index = 0
        cls.__buf_has_init = False

        ret = kd_mpi_vb_exit()
        if ret:
            print(f"buffer_deinit, vb init failed({ret})")
            return ret

        return 0

