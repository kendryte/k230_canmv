from media.vencoder import *
from media.camera import *
from media.media import *
from time import *
import time

def canmv_venc_test():
    width = 1280
    height = 720
    venc_chn = VENC_CHN_ID_0

    width = ALIGN_UP(width, 16)
    camera.sensor_init(CAM_DEV_ID_0, CAM_DEFAULT_SENSOR)

    # set vicap chn0
    camera.set_outbufs(CAM_DEV_ID_0, CAM_CHN_ID_0, 6)
    camera.set_outsize(CAM_DEV_ID_0, CAM_CHN_ID_0, width, height)
    camera.set_outfmt(CAM_DEV_ID_0, CAM_CHN_ID_0, PIXEL_FORMAT_YUV_SEMIPLANAR_420)

    encoder = Encoder()
    ret = encoder.SetOutBufs(venc_chn, 15, width, height)
    if ret:
        print("canmv_venc_test, encoder SetOutBufs failed.")
        return -1

    ret = media.buffer_init()
    if ret:
        print("canmv_venc_test, buffer_init failed.")
        return ret

    chnAttr = ChnAttrStr(encoder.PAYLOAD_TYPE_H265, encoder.H265_PROFILE_MAIN, width, height)
    streamData = StreamData()

    ret = encoder.Create(venc_chn, chnAttr)
    if ret < 0:
        print("canmv_venc_test, vencoder create filed.")
        return ret

    # Bind with camera
    media_source = media_device(CAMERA_MOD_ID, CAM_DEV_ID_0, CAM_CHN_ID_0)
    media_sink = media_device(VIDEO_ENCODE_MOD_ID, VENC_DEV_ID, venc_chn)
    ret = media.create_link(media_source, media_sink)
    if ret:
        print("cam_venc_test, create link with camera failed.")
        return ret

    ret = encoder.Start(venc_chn)
    if ret:
        print("cam_venc_test, encoder start failed")
        return ret

    ret = camera.start_stream(CAM_DEV_ID_0)
    if ret:
        print("cam_venc_test, camera start failed")
        return ret

    frame_count = 0
    if chnAttr.payload_type == encoder.PAYLOAD_TYPE_H265:
        suffix = "265"
    elif chnAttr.payload_type == encoder.PAYLOAD_TYPE_H264:
        suffix = "264"
    else:
        suffix = "unkown"
        print("cam_venc_test, venc payload_type unsupport")
        return -1

    out_file = f"/sdcard/app/tests/venc_chn_{venc_chn:02d}.{suffix}"
    print("save stream to file: ", out_file)

    with open(out_file, "wb") as fo:
        while True:
            ret = encoder.GetStream(venc_chn, streamData)
            if ret < 0:
                print("cam_venc_test, venc get stream failed")
                return ret

            for pack_idx in range(0, streamData.pack_cnt):
                stream_data = uctypes.bytearray_at(streamData.data[pack_idx], streamData.data_size[pack_idx])
                fo.write(stream_data)
                print("stream size: ", streamData.data_size[pack_idx], "stream type: ", streamData.stream_type[pack_idx])

            ret = encoder.ReleaseStream(venc_chn, streamData)
            if ret < 0:
                print("cam_venc_test, venc release stream failed")
                return ret

            frame_count += 1
            if frame_count >= 100:
                break

    camera.stop_stream(CAM_DEV_ID_0)

    ret = media.destroy_link(media_source, media_sink)
    if ret:
        print("cam_venc_test, venc destroy link with camera failed.")
        return ret

    ret = encoder.Stop(venc_chn)
    if ret < 0:
        print("cam_venc_test, venc stop failed.")
        return ret

    ret = encoder.Destroy(venc_chn)
    if ret < 0:
        print("cam_venc_test, venc destroy failed.")
        return ret

    ret = media.buffer_deinit()
    if ret:
        print("cam_venc_test, media buffer deinit failed.")
        return ret


canmv_venc_test()
