from media.vencoder import *
from media.camera import *
from media.media import *
from mpp.mp4_format import *
from mpp.mp4_format_struct import *
from mpp.aenc import *
import uctypes
import time

class MuxerCfgStr:
    def __init__(self):
        self.file_name = 0
        self.video_payload_type = 0
        self.pic_width = 0
        self.pic_height = 0
        self.audio_payload_type = 0
        self.fmp4_flag = 0

class Mp4CfgStr:
    def __init__(self, type):
        self.type = type
        self.muxerCfg = MuxerCfgStr()

    def SetMuxerCfg(self, fileName, videoPayloadType, picWidth, picHeight, audioPayloadType, fmp4Flag = 0):
        self.muxerCfg.file_name = fileName
        self.muxerCfg.video_payload_type = videoPayloadType
        self.muxerCfg.pic_width = picWidth
        self.muxerCfg.pic_height = picHeight
        self.muxerCfg.audio_payload_type = audioPayloadType
        self.muxerCfg.fmp4_flag = fmp4Flag

    def SetDemuxerCfg(self, fileName):
        pass


class Mp4Container:
    MP4_CONFIG_TYPE_MUXER = K_MP4_CONFIG_MUXER
    MP4_CONFIG_TYPE_DEMUXER = K_MP4_CONFIG_DEMUXER

    MP4_STREAM_TYPE_VIDEO = K_MP4_STREAM_VIDEO
    MP4_STREAM_TYPE_AUDIO = K_MP4_STREAM_AUDIO

    MP4_CODEC_ID_H264 = K_MP4_CODEC_ID_H264
    MP4_CODEC_ID_H265 = K_MP4_CODEC_ID_H265
    MP4_CODEC_ID_G711A = K_MP4_CODEC_ID_G711A
    MP4_CODEC_ID_G711U = K_MP4_CODEC_ID_G711U

    def __init__(self):
        camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)
        self.venc = Encoder()
        self.stream_data = StreamData()
        self.mp4_handle = 0
        self.mp4_video_track_handle = 0
        self.mp4_audio_track_handle = 0
        self.video_payload_type = 0
        self.audio_payload_type = 0
        self.get_idr = 0
        self.save_idr = 0
        self.idr_size = 0
        self.idr_pts = 0
        self.count = 0


    def Create(self, mp4Cfg):
        if mp4Cfg.type == K_MP4_CONFIG_MUXER:
            camera.set_outbufs(CAM_DEV_ID_0, CAM_CHN_ID_0, 6)
            camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, mp4Cfg.muxerCfg.pic_width, mp4Cfg.muxerCfg.pic_height)
            camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
            ret = self.venc.SetOutBufs(VENC_CHN_ID_0, 15, mp4Cfg.muxerCfg.pic_width, mp4Cfg.muxerCfg.pic_height)
            if ret:
                print("Mp4Container, venc SetOutBufs failed.")
                return -1

            # audio vb
            config = k_vb_config()
            config.max_pool_cnt = 64
            config.comm_pool[0].blk_cnt = 150
            config.comm_pool[0].blk_size = 8000 * 2 * 4 // 25
            config.comm_pool[0].mode = VB_REMAP_MODE_CACHED
            config.comm_pool[1].blk_cnt = 2
            config.comm_pool[1].blk_size = 8000 * 2 * 4 // 25 * 2
            config.comm_pool[1].mode = VB_REMAP_MODE_CACHED
            media.buffer_config(config)

            ret = media.buffer_init()
            if ret:
                print("Mp4Container, media buffer_init failed.")
                return -1

            self.video_payload_type = mp4Cfg.muxerCfg.video_payload_type
            self.audio_payload_type = mp4Cfg.muxerCfg.audio_payload_type

            profile = self.venc.H265_PROFILE_MAIN
            payload_type = self.venc.PAYLOAD_TYPE_H265
            if mp4Cfg.muxerCfg.video_payload_type == self.MP4_CODEC_ID_H265:
                profile = self.venc.H265_PROFILE_MAIN
                payload_type = self.venc.PAYLOAD_TYPE_H265
            elif mp4Cfg.muxerCfg.video_payload_type == self.MP4_CODEC_ID_H264:
                profile = self.venc.H264_PROFILE_MAIN
                payload_type = self.venc.PAYLOAD_TYPE_H264

            width = ALIGN_UP(mp4Cfg.muxerCfg.pic_width, 16)
            chnAttr = ChnAttrStr(payload_type, profile, width, mp4Cfg.muxerCfg.pic_height)
            self.save_idr = bytearray(width * mp4Cfg.muxerCfg.pic_height * 3 // 4)
            ret = self.venc.Create(VENC_CHN_ID_0, chnAttr)
            if ret:
                print("Mp4Container, venc Create failed.")
                return -1

            aenc_chn_attr = k_aenc_chn_attr()
            if mp4Cfg.muxerCfg.audio_payload_type == self.MP4_CODEC_ID_G711U:
                aenc_chn_attr.type = K_PT_G711U
            elif mp4Cfg.muxerCfg.audio_payload_type == self.MP4_CODEC_ID_G711A:
                aenc_chn_attr.type = K_PT_G711A
            aenc_chn_attr.buf_size = 25
            aenc_chn_attr.point_num_per_frame = 320  # 8000 / 25
            ret = kd_mpi_aenc_create_chn(0, aenc_chn_attr)
            if ret:
                print("Mp4Container, aenc Create failed.")
                return -1

            aio_dev_attr = k_aio_dev_attr()
            aio_dev_attr.audio_type = KD_AUDIO_INPUT_TYPE_I2S
            aio_dev_attr.kd_audio_attr.i2s_attr.sample_rate = 8000
            aio_dev_attr.kd_audio_attr.i2s_attr.bit_width = KD_AUDIO_BIT_WIDTH_16
            aio_dev_attr.kd_audio_attr.i2s_attr.chn_cnt = 2
            aio_dev_attr.kd_audio_attr.i2s_attr.i2s_mode = K_STANDARD_MODE
            aio_dev_attr.kd_audio_attr.i2s_attr.frame_num = 25
            aio_dev_attr.kd_audio_attr.i2s_attr.point_num_per_frame = 320   # 8000 / 25
            aio_dev_attr.kd_audio_attr.i2s_attr.i2s_type = K_AIO_I2STYPE_INNERCODEC
            ret = kd_mpi_ai_set_pub_attr(0, aio_dev_attr)
            if ret:
                print("Mp4Container, ai set pub sttr failed.")
                return -1

            kd_mpi_ai_enable(0)
            kd_mpi_ai_enable_chn(0, 0)

            mp4_cfg = k_mp4_config_s()
            mp4_cfg.config_type = mp4Cfg.type

            mp4_cfg.muxer_config.file_name[:] = bytes(mp4Cfg.muxerCfg.file_name, 'utf-8')
            mp4_cfg.muxer_config.fmp4_flag = mp4Cfg.muxerCfg.fmp4_flag

            handle = k_u64_ptr()
            ret = kd_mp4_create(handle, mp4_cfg)
            if ret:
                print("Mp4Container, kd_mp4_create failed.")
                return -1

            self.mp4_handle = handle.value
            video_track_info = k_mp4_track_info_s()
            video_track_info.track_type = K_MP4_STREAM_VIDEO
            video_track_info.time_scale = 1000
            video_track_info.video_info.width = mp4Cfg.muxerCfg.pic_width
            video_track_info.video_info.height = mp4Cfg.muxerCfg.pic_height
            video_track_info.video_info.codec_id = mp4Cfg.muxerCfg.video_payload_type
            video_track_handle = k_u64_ptr()
            ret = kd_mp4_create_track(self.mp4_handle, video_track_handle, video_track_info)
            if ret:
                print("Mp4Container, kd_mp4_create_track failed.")
                return -1

            self.mp4_video_track_handle = video_track_handle.value

            audio_track_info = k_mp4_track_info_s()
            audio_track_info.track_type = K_MP4_STREAM_AUDIO
            audio_track_info.time_scale = 1000
            audio_track_info.audio_info.channels = 2
            if mp4Cfg.muxerCfg.audio_payload_type == self.MP4_CODEC_ID_G711U:
                audio_track_info.audio_info.codec_id = K_MP4_CODEC_ID_G711U
            elif mp4Cfg.muxerCfg.audio_payload_type == self.MP4_CODEC_ID_G711A:
                audio_track_info.audio_info.codec_id = K_MP4_CODEC_ID_G711A
            audio_track_info.audio_info.sample_rate = 8000
            audio_track_info.audio_info.bit_per_sample = 16
            audio_track_handle = k_u64_ptr()
            ret = kd_mp4_create_track(self.mp4_handle, audio_track_handle, audio_track_info)
            if ret:
                print("Mp4Container, kd_mp4_create_track failed.")
                return -1
            self.mp4_audio_track_handle = audio_track_handle.value

            return 0


    def Start(self):
        media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
        media_sink = media_device(VIDEO_ENCODE_MOD_ID, VENC_DEV_ID, VENC_CHN_ID_0)
        ret = media.create_link(media_source, media_sink)
        if ret:
            print("Mp4Container, create link with camera failed.")
            return -1

        audio_source = media_device(AUDIO_IN_MOD_ID, 0, 0)
        audio_sink = media_device(AUDIO_ENCODE_MOD_ID, 0, 0)
        ret = media.create_link(audio_source, audio_sink)
        if ret:
            print("Mp4Container, create audio link with camera failed.")
            return -1

        ret = self.venc.Start(VENC_CHN_ID_0)
        if ret:
            print("Mp4Container, venc Start failed.")
            return -1

        ret = camera.start_stream(CAM_DEV_ID_0)
        if ret:
            print("Mp4Container, camera start_stream failed.")
            return -1

        return 0


    def Process(self):
        frame_data = k_mp4_frame_data_s()
        ret = self.venc.GetStream(VENC_CHN_ID_0, self.stream_data)
        if ret:
            print("Mp4Container, venc GetStream failed.")
            return -1

        for pack_idx in range(0, self.stream_data.pack_cnt):
            if self.get_idr == 0:
                print(self.idr_size)
                self.save_idr[self.idr_size:self.idr_size+self.stream_data.data_size[pack_idx]] = uctypes.bytearray_at(self.stream_data.data[pack_idx], self.stream_data.data_size[pack_idx])
                print(type(self.save_idr))
                self.idr_size += self.stream_data.data_size[pack_idx]
                self.idr_pts = self.stream_data.pts[pack_idx]
            elif self.get_idr == 1:
                frame_data.codec_id = self.video_payload_type
                frame_data.data = self.stream_data.data[pack_idx]
                frame_data.data_length = self.stream_data.data_size[pack_idx]
                frame_data.time_stamp = self.stream_data.pts[pack_idx]

        if self.get_idr == 0:
            self.count += 1
            if self.count == 2:
                frame_data.codec_id = self.video_payload_type
                frame_data.data = uctypes.addressof(self.save_idr)
                frame_data.data_length = self.idr_size
                frame_data.time_stamp = self.idr_pts
                self.get_idr = 1
            else:
                ret = self.venc.ReleaseStream(VENC_CHN_ID_0, self.stream_data)
                if ret:
                    print("Mp4Container, venc ReleaseStream failed.")
                    return -1
                return 0

        ret = kd_mp4_write_frame(self.mp4_handle, self.mp4_video_track_handle, frame_data)
        if ret:
            print("Mp4Container, kd_mp4_write_frame failed.")
            return -1

        ret = self.venc.ReleaseStream(VENC_CHN_ID_0, self.stream_data)
        if ret:
            print("Mp4Container, venc ReleaseStream failed.")
            return -1

        audio_stream = k_audio_stream()
        ret = kd_mpi_aenc_get_stream(0, audio_stream, 1000)
        if ret:
            print("Mp4Container, get audio stream failed.")
            return -1
        vir_adata = kd_mpi_sys_mmap(audio_stream.phys_addr, audio_stream.len)
        frame_data.codec_id = self.audio_payload_type
        frame_data.data = vir_adata
        frame_data.data_length = audio_stream.len
        frame_data.time_stamp = audio_stream.time_stamp
        ret = kd_mp4_write_frame(self.mp4_handle, self.mp4_audio_track_handle, frame_data)
        if ret:
            print("Mp4Container, write audio stream failed.")
            return -1
        kd_mpi_sys_munmap(vir_adata, audio_stream.len)
        kd_mpi_aenc_release_stream(0, audio_stream)

        return 0


    def Stop(self):
        camera.stop_stream(CAM_DEV_ID_0)

        media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
        media_sink = media_device(VIDEO_ENCODE_MOD_ID, VENC_DEV_ID, VENC_CHN_ID_0)
        ret = media.destroy_link(media_source, media_sink)
        if ret:
            print("Mp4Container, destroy link with camera failed.")
            return -1

        audio_source = media_device(AUDIO_IN_MOD_ID, 0, 0)
        audio_sink = media_device(AUDIO_ENCODE_MOD_ID, 0, 0)
        ret = media.destroy_link(audio_source, audio_sink)
        if ret:
            print("Mp4Container, destroy audio link with camera failed.")
            return -1

        kd_mpi_ai_disable_chn(0, 0)
        kd_mpi_ai_disable(0)

        self.venc.Stop(VENC_CHN_ID_0)
        if ret:
            print("Mp4Container, venc Stop failed.")
            return -1

        return 0


    def Destroy(self):
        ret = kd_mp4_destroy_tracks(self.mp4_handle)
        if ret:
            print("Mp4Container, kd_mp4_destroy_tracks failed.")
            return -1

        ret = kd_mp4_destroy(self.mp4_handle)
        if ret:
            print("Mp4Container, kd_mp4_destroy failed.")
            return -1

        ret = self.venc.Destroy(VENC_CHN_ID_0)
        if ret:
            print("Mp4Container, venc Destroy failed.")
            return -1

        kd_mpi_aenc_destroy_chn(0)

        ret = media.buffer_deinit()
        if ret:
            print("Mp4Container, media buffer_init failed.")
            return -1

        return 0