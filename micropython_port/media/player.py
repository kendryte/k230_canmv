from media.media import *
from mpp.mp4_format import *
from mpp.mp4_format_struct import *
from media.pyaudio import *
import media.g711 as g711
from mpp.payload_struct import *
import media.vdecoder as vdecoder
from media.display import *
import uctypes
import time
import _thread

K_PLAYER_EVENT_EOF = 0
K_PLAYER_EVENT_PROGRESS = 1
DIV = 25

PLAY_START = 0
PLAY_STOP = 1
PLAY_PAUSE = 2

class Player:
    def __init__(self):
        self.mp4_cfg = k_mp4_config_s()
        self.video_info = k_mp4_video_info_s()
        self.video_track = False
        self.audio_info = k_mp4_audio_info_s()
        self.audio_track = False
        self.mp4_handle = k_u64_ptr()
        self.play_status = PLAY_STOP

    def _init_media_buffer(self):
        if (self.audio_track):
            CHUNK = self.audio_info.sample_rate//DIV
            self.pyaudio = PyAudio()
            self.pyaudio.initialize(CHUNK)
            if (self.audio_info.codec_id == K_MP4_CODEC_ID_G711A):
                self.adec = g711.Decoder(K_PT_G711A,CHUNK)
            elif (self.audio_info.codec_id == K_MP4_CODEC_ID_G711U):
                self.adec = g711.Decoder(K_PT_G711U,CHUNK)

        if (self.video_track):
            if (self.video_info.codec_id == K_MP4_CODEC_ID_H264):
                self.vdec = vdecoder.Decoder(K_PT_H264)
            elif (self.video_info.codec_id == K_MP4_CODEC_ID_H265):
                self.vdec = vdecoder.Decoder(K_PT_H265)

        ret = media.buffer_init()
        if ret:
            raise ValueError("player buffer_init failed")

        if (self.audio_track):
            self.adec.create()

        if (self.video_track):
            self.vdec.create()
            display.init(LT9611_1920X1080_30FPS)
            if (self.video_info.width >= 1920):
                display.set_plane(0, 0, self.video_info.width, self.video_info.height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)
            else:
                display.set_plane(400, 200, self.video_info.width, self.video_info.height, PIXEL_FORMAT_YVU_PLANAR_420, DISPLAY_MIRROR_NONE, DISPLAY_CHN_VIDEO1)
        return 0

    def _deinit_media_buffer(self):
        if (self.video_track):
            self.vdec.destroy()

        if (self.audio_track):
            self.pyaudio.terminate()
            self.adec.destroy()

        if (self.video_track):
            time.sleep(1)
            display.deinit()

        self.video_track = False
        self.audio_track = False
        media.buffer_deinit()
        return 0


    def _do_file_data(self):
        frame_data =  k_mp4_frame_data_s()
        while(self.play_status == PLAY_START or self.play_status == PLAY_PAUSE):
            if (self.play_status == PLAY_PAUSE):
                time.sleep(0.1)
            else:
                ret = kd_mp4_get_frame(self.mp4_handle.value, frame_data)
                if (ret < 0 ):
                    raise ValueError("get frame data failed")

                if (frame_data.eof):
                    print("demuxer finished.")
                    break

                if (frame_data.codec_id == K_MP4_CODEC_ID_H264 or frame_data.codec_id == K_MP4_CODEC_ID_H265):
                    data = uctypes.bytes_at(frame_data.data,frame_data.data_length)
                    self.vdec.decode(data)
                elif(frame_data.codec_id == K_MP4_CODEC_ID_G711A or frame_data.codec_id == K_MP4_CODEC_ID_G711U):
                    data = uctypes.bytes_at(frame_data.data,frame_data.data_length)
                    self.audio_out_stream.write(self.adec.decode(data))

        self.callback(K_PLAYER_EVENT_EOF,0)

    def debug_codec_info(self):
        if (self.video_track):
            if (self.video_info.codec_id == K_MP4_CODEC_ID_H264):
                print("video track h264")
            elif (self.video_info.codec_id == K_MP4_CODEC_ID_H265):
                print("video track h265")

        if (self.audio_track):
            if (self.audio_info.codec_id == K_MP4_CODEC_ID_G711A):
                print("audio track g711a")
            elif (self.audio_info.codec_id == K_MP4_CODEC_ID_G711U):
                print("audio track g711u")

    def load(self,filename):
        self.mp4_cfg.config_type = K_MP4_CONFIG_DEMUXER
        self.mp4_cfg.muxer_config.file_name[:] = bytes(filename, 'utf-8')
        self.mp4_cfg.muxer_config.fmp4_flag = 0
        ret = kd_mp4_create(self.mp4_handle, self.mp4_cfg)
        if ret:
            print("kd_mp4_create failed:",filename)
            return -1

        file_info = k_mp4_file_info_s()
        kd_mp4_get_file_info(self.mp4_handle.value, file_info)
        #print("=====file_info: track_num:",file_info.track_num,"duration:",file_info.duration)

        for i in range(file_info.track_num):
            track_info = k_mp4_track_info_s()
            ret = kd_mp4_get_track_by_index(self.mp4_handle.value, i, track_info)
            if (ret < 0):
                raise ValueError("kd_mp4_get_track_by_index failed")

            if (track_info.track_type == K_MP4_STREAM_VIDEO):
                if (track_info.video_info.codec_id == K_MP4_CODEC_ID_H264 or track_info.video_info.codec_id == K_MP4_CODEC_ID_H265):
                    self.video_track = True
                    self.video_info = track_info.video_info
                    print("    codec_id: ", self.video_info.codec_id)
                    print("    track_id: ", self.video_info.track_id)
                    print("    width: ", self.video_info.width)
                    print("    height: ", self.video_info.height)
                else:
                    print("video not support codecid:",track_info.video_info.codec_id)
            elif (track_info.track_type == K_MP4_STREAM_AUDIO):
                if (track_info.audio_info.codec_id == K_MP4_CODEC_ID_G711A or track_info.audio_info.codec_id == K_MP4_CODEC_ID_G711U):
                    self.audio_track = True
                    self.audio_info = track_info.audio_info
                    print("    codec_id: ", self.audio_info.codec_id)
                    print("    track_id: ", self.audio_info.track_id)
                    print("    channels: ", self.audio_info.channels)
                    print("    sample_rate: ", self.audio_info.sample_rate)
                    print("    bit_per_sample: ", self.audio_info.bit_per_sample)
                    #self.audio_info.channels = 2
                else:
                    print("audio not support codecid:",track_info.audio_info.codec_id)

        self.debug_codec_info()
        return 0

    def start(self):
        if (0 != self._init_media_buffer()):
            raise ValueError("_init_media_buffer failed")
            return -1

        if (self.video_track):
            self.vdec.start()
            self.video_meida_source = media_device(VIDEO_DECODE_MOD_ID, VDEC_DEV_ID, self.vdec.get_vdec_channel())
            self.video_meida_sink = media_device(DISPLAY_MOD_ID, DISPLAY_DEV_ID, DISPLAY_CHN_VIDEO1)
            media.create_link(self.video_meida_source, self.video_meida_sink)

        if (self.audio_track):
            self.audio_out_stream = self.pyaudio.open(format=paInt16,
                                        channels=self.audio_info.channels,
                                        rate=self.audio_info.sample_rate,
                                        output=True,
                                        frames_per_buffer=self.audio_info.sample_rate//DIV)

        self.play_status = PLAY_START
        #self._do_file_data()
        _thread.start_new_thread(self._do_file_data,())


    def stop(self):
        self.play_status = PLAY_STOP
        if (self.video_track):
            self.vdec.stop()
            media.destroy_link(self.video_meida_source, self.video_meida_sink)

        ret = kd_mp4_destroy(self.mp4_handle.value)
        if (ret < 0):
            print("destroy mp4 failed.")

        if (self.audio_track):
            self.audio_out_stream.stop_stream()
            self.audio_out_stream.close()

        self._deinit_media_buffer()

    def pause(self):
        self.play_status = PLAY_PAUSE

    def resume(self):
        pself.play_status = PLAY_START

    def set_event_callback(self,callback):
        self.callback = callback


    def destroy_mp4(self):
        ret = kd_mp4_destroy(self.mp4_handle.value)
        if (ret < 0):
            print("destroy mp4 failed.")
        else:
            print("destroy mp4 ok.")
