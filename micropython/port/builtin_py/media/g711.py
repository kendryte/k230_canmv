import time
import uctypes
from mpp import *
from mpp.vb import *
from mpp.sys import *
from mpp.payload_struct import *
from mpp.adec_struct import *
from mpp.aenc_struct import *
from media.media import *

BUF_CNT = 5

def _vb_buffer_init(frames_per_buffer=1024):
    config = k_vb_config()
    config.max_pool_cnt = 1
    config.comm_pool[0].blk_cnt = BUF_CNT
    config.comm_pool[0].blk_size = int(frames_per_buffer * 2 * 4)
    config.comm_pool[0].mode = VB_REMAP_MODE_NOCACHE

    MediaManager._config(config)

class Encoder:
    chns_enable = [0 for i in range(0,AENC_MAX_CHN_NUMS)]

    def _init_audio_frame(self):
        if self.buffer is None:
            frame_size = int(self.point_num_per_frame*2*2)

            self.buffer = MediaManager.Buffer.get(frame_size)

            self._audio_frame.len = frame_size
            self._audio_frame.phys_addr = self.buffer.phys_addr
            self._audio_frame.virt_addr = self.buffer.virt_addr

    def _deinit_audio_frame(self):
        self.buffer.__del__()
        self.buffer = None

    @classmethod
    def get_free_chn_index(cls):
        for i in cls.chns_enable:
            if (i == 0):
                return i
        return -1

    def __init__(self,type,frames_per_buffer):
        self.type = type
        self.point_num_per_frame = frames_per_buffer
        self.chn = -1
        self.init = False
        self.buffer = None
        self._audio_frame = k_audio_frame()
        _vb_buffer_init(frames_per_buffer)

    def create(self):
        if (not self.init):
            self._init_audio_frame()
            chn_index = Encoder.get_free_chn_index()
            if (chn_index == -1):
                self._deinit_audio_frame()
                raise ValueError("Encoder has no free chn")
            else:
                Encoder.chns_enable[chn_index] = 1

            self.chn = chn_index
            aenc_chn_attr = k_aenc_chn_attr()
            aenc_chn_attr.type = self.type
            aenc_chn_attr.buf_size = BUF_CNT
            aenc_chn_attr.point_num_per_frame = self.point_num_per_frame

            if (0 != kd_mpi_aenc_create_chn(self.chn, aenc_chn_attr)):
                self._deinit_audio_frame()
                raise ValueError(("kd_mpi_aenc_create_chn:%d faild")%(self.chn))

            self.init = True
        else:
            raise RuntimeError("The instance has been initialized")

    def destroy(self):
        if (self.init):
            if (0 != kd_mpi_aenc_destroy_chn(self.chn)):
                self._deinit_audio_frame()
                raise ValueError(("kd_mpi_aenc_destroy_chn:%d faild")%(self.chn))
            else:
                Encoder.chns_enable[self.chn] = 0

            self._deinit_audio_frame()

        self.init = False

    def encode(self,frame_data):
        audio_stream = k_audio_stream()
        self._audio_frame.len = len(frame_data)
        uctypes.bytearray_at(self._audio_frame.virt_addr, self._audio_frame.len)[:] = frame_data

        if (0 != kd_mpi_aenc_send_frame(self.chn, self._audio_frame)):
            raise ValueError(("kd_mpi_aenc_send_frame:%d faild")%(self.chn))

        if (0 != kd_mpi_aenc_get_stream(self.chn, audio_stream, 1000)):
            raise ValueError(("kd_mpi_aenc_get_stream:%d faild")%(self.chn))

        vir_data = kd_mpi_sys_mmap(audio_stream.phys_addr, audio_stream.len)
        data = uctypes.bytes_at(vir_data,audio_stream.len)
        kd_mpi_sys_munmap(vir_data,audio_stream.len)
        kd_mpi_aenc_release_stream(self.chn, audio_stream)

        return data


class Decoder:
    chns_enable = [0 for i in range(0,ADEC_MAX_CHN_NUMS)]

    def _init_audio_stream(self):
        if self.buffer is None:
            frame_size = int(self.point_num_per_frame*2*2)

            self.buffer = MediaManager.Buffer.get(frame_size)

            self._audio_stream.len = frame_size
            self._audio_stream.phys_addr = self.buffer.phys_addr
            self._audio_stream.stream = self.buffer.virt_addr

    def _deinit_audio_stream(self):
        self.buffer.__del__()
        self.buffer = None

    @classmethod
    def get_free_chn_index(cls):
        for i in cls.chns_enable:
            if (i == 0):
                return i
        return -1

    def __init__(self,type,frames_per_buffer):
        self.type = type
        self.point_num_per_frame = frames_per_buffer
        self.chn = -1
        self.init = False
        self.buffer = None
        self._audio_stream = k_audio_stream()
        _vb_buffer_init(frames_per_buffer)

    def create(self):
        if (not self.init):
            self._init_audio_stream()
            chn_index = Decoder.get_free_chn_index()
            if (chn_index == -1):
                self._deinit_audio_stream()
                raise ValueError("Decoder has no free chn")
            else:
                Decoder.chns_enable[chn_index] = 1

            self.chn = chn_index
            adec_chn_attr = k_adec_chn_attr()
            adec_chn_attr.type = self.type
            adec_chn_attr.buf_size = BUF_CNT
            adec_chn_attr.point_num_per_frame = self.point_num_per_frame

            if (0 != kd_mpi_adec_create_chn(self.chn, adec_chn_attr)):
                self._deinit_audio_stream()
                raise ValueError(("kd_mpi_adec_create_chn:%d faild")%(self.chn))

            self.init = True
        else:
            raise RuntimeError("The instance has been initialized")

    def destroy(self):
        if (self.init):
            if (0 != kd_mpi_adec_destroy_chn(self.chn)):
                self._deinit_audio_stream()
                raise ValueError(("kd_mpi_adec_destroy_chn:%d faild")%(self.chn))
            else:
                Decoder.chns_enable[self.chn] = 0
            self._deinit_audio_stream()

        self.init = False

    def decode(self,stream_data):
        audio_frame = k_audio_frame()
        self._audio_stream.len = len(stream_data)
        uctypes.bytearray_at(self._audio_stream.stream, self._audio_stream.len)[:] = stream_data

        if (0 != kd_mpi_adec_send_stream(self.chn, self._audio_stream,True)):
            raise ValueError(("kd_mpi_adec_send_stream:%d faild")%(self.chn))

        if (0 != kd_mpi_adec_get_frame(self.chn, audio_frame, 1000)):
            raise ValueError(("kd_mpi_adec_get_frame:%d faild")%(self.chn))

        vir_data = kd_mpi_sys_mmap(audio_frame.phys_addr, audio_frame.len)
        data = uctypes.bytes_at(vir_data,audio_frame.len)
        kd_mpi_sys_munmap(vir_data,audio_frame.len)
        kd_mpi_adec_release_frame(self.chn, audio_frame)

        return data


