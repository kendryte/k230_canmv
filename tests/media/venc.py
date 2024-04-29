# Video encode example
#
# Note: You will need an SD card to run this example.
#
# You can capture videos and encode them into 264 files

from media.vencoder import *
from media.camera import *
from media.media import *
import time, os

def venc_test():
    print("venc_test start")
    width = 1280
    height = 720
    venc_chn = VENC_CHN_ID_0
    width = ALIGN_UP(width, 16)
    # 初始化sensor
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)
    # 设置camera 输出buffer
    camera.set_outbufs(CAM_DEV_ID_0, CAM_CHN_ID_0, 6)
    # 设置camera 输出buffer size,4k alignment
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, width, height,12)
    # 设置camera 输出格式
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)
    # 实例化video encoder
    encoder = Encoder()
    # 设置video encoder 输出buffer
    encoder.SetOutBufs(venc_chn, 15, width, height)
    # 初始化设置的buffer
    media.buffer_init()
    chnAttr = ChnAttrStr(encoder.PAYLOAD_TYPE_H265, encoder.H265_PROFILE_MAIN, width, height)
    streamData = StreamData()
    # 创建编码器
    encoder.Create(venc_chn, chnAttr)
    # 绑定camera和venc
    media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    media_sink = media_device(VIDEO_ENCODE_MOD_ID, VENC_DEV_ID, venc_chn)
    media.create_link(media_source, media_sink)
    # 开始编码
    encoder.Start(venc_chn)
    # 启动camera
    camera.start_stream(CAM_DEV_ID_0)

    frame_count = 0
    if chnAttr.payload_type == encoder.PAYLOAD_TYPE_H265:
        suffix = "265"
    elif chnAttr.payload_type == encoder.PAYLOAD_TYPE_H264:
        suffix = "264"
    else:
        suffix = "unkown"
        print("cam_venc_test, venc payload_type unsupport")

    out_file = f"/sdcard/app/tests/venc_chn_{venc_chn:02d}.{suffix}"
    print("save stream to file: ", out_file)

    with open(out_file, "wb") as fo:
        try:
            while True:
                os.exitpoint()
                encoder.GetStream(venc_chn, streamData) # 获取一帧码流

                for pack_idx in range(0, streamData.pack_cnt):
                    stream_data = uctypes.bytearray_at(streamData.data[pack_idx], streamData.data_size[pack_idx])
                    fo.write(stream_data) # 码流写文件
                    print("stream size: ", streamData.data_size[pack_idx], "stream type: ", streamData.stream_type[pack_idx])

                encoder.ReleaseStream(venc_chn, streamData) # 释放一帧码流

                frame_count += 1
                if frame_count >= 100:
                    break
        except KeyboardInterrupt as e:
            print("user stop: ", e)
        except BaseException as e:
            sys.print_exception(e)

    # 停止camera
    camera.stop_stream(CAM_DEV_ID_0)
    # 销毁camera和venc的绑定
    media.destroy_link(media_source, media_sink)
    # 停止编码
    encoder.Stop(venc_chn)
    # 销毁编码器
    encoder.Destroy(venc_chn)
    # 清理buffer
    media.buffer_deinit()
    print("venc_test stop")

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    venc_test()
