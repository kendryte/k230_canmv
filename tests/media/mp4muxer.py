# Save MP4 file example
#
# Note: You will need an SD card to run this example.
#
# You can capture audio and video and save them as MP4.The current version only supports MP4 format, video supports 264/265, and audio supports g711a/g711u.

from media.mp4format import *

def canmv_mp4_muxer_test():
    width = 1280
    height = 720
    mp4_muxer = Mp4Container()

    mp4_cfg = Mp4CfgStr(mp4_muxer.MP4_CONFIG_TYPE_MUXER)
    if mp4_cfg.type == mp4_muxer.MP4_CONFIG_TYPE_MUXER:
        file_name = "/sdcard/app/tests/test.mp4"
        mp4_cfg.SetMuxerCfg(file_name, mp4_muxer.MP4_CODEC_ID_H265, width, height, mp4_muxer.MP4_CODEC_ID_G711U)

    ret = mp4_muxer.Create(mp4_cfg)
    if ret:
        print("canmv_mp4_muxer_test, mp4 muxer Create failed.")
        return -1

    ret = mp4_muxer.Start()
    if ret:
        print("canmv_mp4_muxer_test, mp4 muxer Start failed.")
        return -1

    frame_count = 0

    while True:
        ret = mp4_muxer.Process()
        if ret:
            print("canmv_mp4_muxer_test, mp4 muxer Process failed.")
            return -1

        frame_count += 1
        print("frame_coutn = ", frame_count)
        if frame_count >= 100:
            break

    ret = mp4_muxer.Stop()
    if ret:
        print("canmv_mp4_muxer_test, mp4 muxer Stop failed.")
        return -1

    ret = mp4_muxer.Destroy()
    if ret:
        print("canmv_mp4_muxer_test, mp4 muxer Destroy failed.")
        return -1

    return 0

canmv_mp4_muxer_test()