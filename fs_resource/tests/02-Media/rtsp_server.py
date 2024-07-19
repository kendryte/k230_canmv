# Description: This example demonstrates how to stream video and audio to the network using the RTSP server.
#
# Note: You will need an SD card to run this example.
#
# You can run the rtsp server to stream video and audio to the network

from time import *
from media.rtspserver import *   #导入rtsp server 模块
import os
import time

def rtsp_server_test():
    rtspserver = RtspServer() #创建rtsp server对象
    rtspserver.start() #启动rtsp server
    print("rtsp server start:",rtspserver.get_rtsp_url()) #打印rtsp server start

    time_start = time.time() #获取当前时间
    try:
        while(time.time() - time_start < 30):
            time.sleep(0.1)
            os.exitpoint()
    except KeyboardInterrupt as e:
        print("user stop: ", e)
    except BaseException as e:
        sys.print_exception(e)

    rtspserver.stop() #停止rtsp server
    print("rtsp server stop") #打印rtsp server stop

if __name__ == "__main__":
    os.exitpoint(os.EXITPOINT_ENABLE)
    rtsp_server_test()
    print("rtsp server done")