import uctypes
from mpp.venc import *
from mpp.sys import *
from mpp.payload_struct import *
from mpp.venc_struct import *
from media.media import *
from mpp.video_struct import *

class ChnAttrStr:
    def __init__(self, payloadType, profile, picWidth, picHeight, gopLen = 30):
        self.payload_type = payloadType
        self.profile = profile
        self.pic_width = picWidth
        self.pic_height = picHeight
        self.gop_len = gopLen

class StreamData:
    def __init__(self):
        self.data = [0 for i in range(0, VENC_PACK_CNT_MAX)]
        self.phy_addr = [0 for i in range(0, VENC_PACK_CNT_MAX)]
        self.data_size = [0 for i in range(0, VENC_PACK_CNT_MAX)]
        self.stream_type = [0 for i in range(0, VENC_PACK_CNT_MAX)]
        self.pts = [0 for i in range(0, VENC_PACK_CNT_MAX)]
        self.pack_cnt = 0

class Encoder:
    PAYLOAD_TYPE_H264 = K_PT_H264
    PAYLOAD_TYPE_H265 = K_PT_H265

    H264_PROFILE_BASELINE = VENC_PROFILE_H264_BASELINE
    H264_PROFILE_MAIN = VENC_PROFILE_H264_MAIN
    H264_PROFILE_HIGH = VENC_PROFILE_H264_HIGH
    H265_PROFILE_MAIN = VENC_PROFILE_H265_MAIN

    STREAM_TYPE_HEADER = K_VENC_HEADER
    STREAM_TYPE_I = K_VENC_I_FRAME
    STREAM_TYPE_P = K_VENC_P_FRAME

    def __init__(self):
        self.output = k_venc_stream()
        self.outbuf_num = 0

    def SetOutBufs(self, chn, buf_num, width, height):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc create, chn id: ", chn, " out of range 0 ~ 3")

        if buf_num and width and height:
            self.outbuf_num = buf_num
            config = k_vb_config()
            config.max_pool_cnt = 1
            config.comm_pool[0].blk_cnt = buf_num
            config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE
            config.comm_pool[0].blk_size = ALIGN_UP(width * height * 3 // 4, VENC_ALIGN_4K)
            MediaManager._config(config)

    def Create(self, chn, chnAttr):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc create, chn id: ", chn, " out of range 0 ~ 3")

        venc_chn_attr = k_venc_chn_attr()
        venc_chn_attr.venc_attr.type = chnAttr.payload_type
        venc_chn_attr.venc_attr.stream_buf_size = ALIGN_UP(chnAttr.pic_width * chnAttr.pic_height * 3 // 4, VENC_ALIGN_4K)
        venc_chn_attr.venc_attr.stream_buf_cnt = self.outbuf_num
        venc_chn_attr.venc_attr.pic_width = chnAttr.pic_width
        venc_chn_attr.venc_attr.pic_height = chnAttr.pic_height
        venc_chn_attr.venc_attr.profile = chnAttr.profile

        venc_chn_attr.rc_attr.rc_mode = K_VENC_RC_MODE_CBR
        venc_chn_attr.rc_attr.cbr.gop = chnAttr.gop_len
        venc_chn_attr.rc_attr.cbr.stats_time = 0
        venc_chn_attr.rc_attr.cbr.src_frame_rate = 30
        venc_chn_attr.rc_attr.cbr.dst_frame_rate = 30
        venc_chn_attr.rc_attr.cbr.bit_rate = 4000

        ret = kd_mpi_venc_create_chn(chn, venc_chn_attr)
        if ret != 0:
            raise OSError("mpi venc create chn failed.")

    def Start(self, chn):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc Start, chn id: ", chn, " out of range 0 ~ 3")

        ret = kd_mpi_venc_start_chn(chn)
        if ret != 0:
            raise OSError("mpi venc start failed.")

    def GetStream(self, chn, streamData):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc GetStream, chn id: ", chn, " out of range 0 ~ 3")

        status = k_venc_chn_status()
        ret = kd_mpi_venc_query_status(chn, status)
        if ret != 0:
            raise OSError("mpi venc query status failed.")

        self.output.pack_cnt = 1

        streamData.pack_cnt = self.output.pack_cnt

        buf = bytearray(uctypes.sizeof(venc_def.k_venc_pack_desc, uctypes.NATIVE) * self.output.pack_cnt)
        self.output.pack = uctypes.addressof(buf)

        ret = kd_mpi_venc_get_stream(chn, self.output, 1000)
        if ret != 0:
            raise OSError("mpi venc get stream failed.")

        for pack_idx in range(0, streamData.pack_cnt):
            vir_data = kd_mpi_sys_mmap(self.output._pack[pack_idx].phys_addr, self.output._pack[pack_idx].len)
            streamData.data[pack_idx] = vir_data
            streamData.data_size[pack_idx] = self.output._pack[pack_idx].len
            streamData.stream_type[pack_idx] = self.output._pack[pack_idx].type
            streamData.pts[pack_idx] = self.output._pack[pack_idx].pts
            streamData.phy_addr[pack_idx] = self.output._pack[pack_idx].phys_addr

    def ReleaseStream(self, chn, streamData):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc ReleaseStream, chn id: ", chn, " out of range 0 ~ 3")

        for pack_idx in range(0, streamData.pack_cnt):
            ret = kd_mpi_sys_munmap(streamData.data[pack_idx], streamData.data_size[pack_idx])
            if ret != 0:
                raise OSError("mpi sys munmap failed.")

        ret = kd_mpi_venc_release_stream(chn, self.output)
        if ret != 0:
            raise OSError("mpi venc release stream failed.")

    def SendFrame(self, chn, frame):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc SendFrame, chn id: ", chn, " out of range 0 ~ 3")

        ret = kd_mpi_venc_send_frame(chn, frame, 1000)
        if ret != 0:
            raise OSError("mpi venc send frame failed.")

    def Stop(self, chn):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc Stop, chn id: ", chn, " out of range 0 ~ 3")

        ret = kd_mpi_venc_stop_chn(chn)
        if ret != 0:
            raise OSError("mpi venc stop failed.")

    def Destroy(self, chn):
        if (chn > VENC_CHN_ID_MAX - 1):
            raise ValueError("venc Destroy, chn id: ", chn, " out of range 0 ~ 3")

        ret = kd_mpi_venc_destroy_chn(chn)
        if ret != 0:
            raise OSError("mpi venc destroy failed.")
