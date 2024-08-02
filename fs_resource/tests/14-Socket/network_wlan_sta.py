import network

def sta_test():
    sta=network.WLAN(0)
    if(sta.active() == False):
        sta.active(1)
    #查看sta是否激活
    print(sta.active())
    #查看sta状态
    print(sta.status())
    #sta连接ap
    print(sta.connect("Canaan","Canaan314"))
    #状态
    print(sta.status())
    #查看ip配置
    print(sta.ifconfig())
    #查看是否连接
    print(sta.isconnected())
    #断开连接
    print(sta.disconnect())
    #连接ap
    print(sta.connect("Canaan","Canaan314"))
    #查看状态
    print(sta.status())

sta_test()

