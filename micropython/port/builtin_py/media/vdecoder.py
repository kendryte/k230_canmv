import os
import time
import uctypes
from mpp.vdec import *
from mpp.sys import *
from mpp.payload_struct import *
from mpp.vdec_struct import *
from media.media import *

# MAX_WIDTH = 1088
# MAX_HEIGHT = 1920
MAX_WIDTH = 1920
MAX_HEIGHT = 1088
STREAM_BUF_SIZE = MAX_WIDTH*MAX_HEIGHT
FRAME_BUF_SIZE = MAX_WIDTH*MAX_HEIGHT*2
INPUT_BUF_CNT = 4
OUTPUT_BUF_CNT = 6

class Decoder:
    input_pool_id = [-1 for i in range(0,VDEC_MAX_CHN_NUMS)]
    output_pool_id = [-1 for i in range(0,VDEC_MAX_CHN_NUMS)]
    chns_enable = [0 for i in range(0,VDEC_MAX_CHN_NUMS)]

    @classmethod
    def vb_create_pool(cls,chn):
        pool_config = k_vb_pool_config()
        pool_config.blk_cnt = INPUT_BUF_CNT
        pool_config.blk_size = STREAM_BUF_SIZE
        pool_config.mode = VB_REMAP_MODE_NOCACHE
        cls.input_pool_id[chn] = kd_mpi_vb_create_pool(pool_config)
        print("input_pool_id ",cls.input_pool_id[chn])

        pool_config.blk_cnt = OUTPUT_BUF_CNT
        pool_config.blk_size = FRAME_BUF_SIZE
        pool_config.mode = VB_REMAP_MODE_NOCACHE
        cls.output_pool_id[chn] = kd_mpi_vb_create_pool(pool_config)
        print("output_pool_id ", cls.output_pool_id[chn])

    @classmethod
    def vb_destory_pool(cls, chn):
        print("destory_pool input ", cls.input_pool_id[chn])
        kd_mpi_vb_destory_pool(cls.input_pool_id[chn])
        print("destory_pool output ", cls.output_pool_id[chn])
        kd_mpi_vb_destory_pool(cls.output_pool_id[chn])

    @classmethod
    def get_free_chn_index(cls):
        for i in cls.chns_enable:
            if (i == 0):
                return i
        return -1

    @classmethod
    def find_use_chn_index(cls):
        for i in cls.chns_enable:
            if (i == 1):
                return i
        return -1

    def __init__(self,type):
        self.chn = -1
        self.type = type

    def create(self):
        chn_index = Decoder.get_free_chn_index()
        if (chn_index == -1):
            raise OSError("Decoder has no free chn")
        else:
            Decoder.chns_enable[chn_index] = 1

        self.chn = chn_index
        Decoder.vb_create_pool(self.chn)

        attr = k_vdec_chn_attr()
        attr.pic_width = MAX_WIDTH
        attr.pic_height = MAX_HEIGHT
        attr.frame_buf_cnt = OUTPUT_BUF_CNT
        attr.frame_buf_size = FRAME_BUF_SIZE
        attr.stream_buf_size = STREAM_BUF_SIZE
        attr.type = self.type
        attr.frame_buf_pool_id = Decoder.output_pool_id[self.chn]

        ret = kd_mpi_vdec_create_chn(self.chn, attr)
        if (ret != 0):
            raise OSError("kd_mpi_vdec_create_chn failed,channel:",self.chn)

    def start(self):
        ret = kd_mpi_vdec_start_chn(self.chn)
        if (ret != 0):
            raise OSError("kd_mpi_vdec_start_chn failed,channel:",self.chn)

    def stop(self):
        stream_data = b'\x00\x00\x00\x01\x64'
        stream = k_vdec_stream()
        stream.end_of_stream = True
        stream.pts = 0

        # blk_size = STREAM_BUF_SIZE
        # poolid = Decoder.input_pool_id[self.chn]

        # buffer = MediaManager.Buffer.get(blk_size, poolid)

        # stream.len = len(stream_data)
        # stream.phy_addr = buffer.phys_addr
        # uctypes.bytearray_at(buffer.virt_addr, stream.len)[:] = stream_data

        poolid = Decoder.input_pool_id[self.chn]
        blk_size = STREAM_BUF_SIZE
        for i in range(10):
            handle = kd_mpi_vb_get_block(poolid, blk_size, "")
            if (handle == -1):
                pass
            else:
                break

        phys_addr = kd_mpi_vb_handle_to_phyaddr(handle)
        vir_data = kd_mpi_sys_mmap(phys_addr, blk_size)

        stream.len = len(stream_data)
        uctypes.bytearray_at(vir_data, stream.len)[:] = stream_data
        stream.phy_addr = phys_addr

        kd_mpi_sys_munmap(vir_data,blk_size)
        kd_mpi_vb_release_block(handle)

        ret = kd_mpi_vdec_send_stream(self.chn, stream, -1)
        if (ret != 0):
            del buffer
            raise OSError("kd_mpi_vdec_send_stream failed,channel:",self.chn)
        # del buffer

        status = k_vdec_chn_status()
        os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
        while(1):
            kd_mpi_vdec_query_status(self.chn, status)
            if (status.end_of_stream == True):
                #print("kd_mpi_vdec_query_status end")
                break
            else:
                time.sleep(0.01)
        os.exitpoint(os.EXITPOINT_ENABLE)

        ret = kd_mpi_vdec_stop_chn(self.chn)
        if (ret != 0):
            raise OSError("kd_mpi_vdec_stop_chn failed,channel:",self.chn)

    def decode(self,stream_data):
        stream = k_vdec_stream()
        stream.end_of_stream = False

        blk_size = STREAM_BUF_SIZE
        poolid = Decoder.input_pool_id[self.chn]

        # buffer = MediaManager.Buffer.get(blk_size, poolid)

        # stream.len = len(stream_data)
        # stream.phy_addr = buffer.phys_addr
        # uctypes.bytearray_at(buffer.virt_addr, stream.len)[:] = stream_data
        for i in range(10):
            handle = kd_mpi_vb_get_block(poolid, blk_size, "")
            if (handle == -1):
                pass
            else:
                break

        phys_addr = kd_mpi_vb_handle_to_phyaddr(handle)
        vir_data = kd_mpi_sys_mmap(phys_addr, blk_size)

        stream.len = len(stream_data)
        uctypes.bytearray_at(vir_data, stream.len)[:] = stream_data
        stream.phy_addr = phys_addr

        kd_mpi_sys_munmap(vir_data,blk_size)

        ret = kd_mpi_vdec_send_stream(self.chn, stream, -1)
        if (ret != 0):
            #del buffer
            raise OSError("kd_mpi_vdec_send_stream failed,channel:",self.chn)
        # del buffer
        kd_mpi_vb_release_block(handle)

    def destroy(self):
        ret = kd_mpi_vdec_destroy_chn(self.chn)
        if (ret != 0):
            raise OSError("kd_mpi_vdec_destroy_chn failed,channel:",self.chn)
        Decoder.chns_enable[self.chn] = 0
        Decoder.vb_destory_pool(self.chn)

    def get_vdec_channel(self):
        return self.chn

    def bind_info(self, x = 0, y = 0, width = 1920,height = 1080,pix_format=PIXEL_FORMAT_YUV_SEMIPLANAR_420,chn = 0):
        if (chn > VDEC_MAX_CHN_NUMS - 1):
            raise AssertionError(f"invaild chn id {chn}, should < {VDEC_MAX_CHN_NUMS - 1}")

        kwargs = {
            'src': (VIDEO_DECODE_MOD_ID, VDEC_DEV_ID, chn),
            'rect': (x, y, width, height),
            'pix_format': pix_format,
        }

        return kwargs
