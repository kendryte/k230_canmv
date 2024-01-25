import network

def sta_test():
    sta=network.WLAN(0)
    #查看sta是否激活
    print(sta.active())
    #查看sta状态
    print(sta.status())
    #扫描并打印结果
    print(sta.scan())
    #sta连接ap
    print(sta.connect("wjx_pc","12345678"))
    #状态
    print(sta.status())
    #查看ip配置
    print(sta.ifconfig())
    #查看是否连接
    print(sta.isconnected())
    #断开连接
    print(sta.disconnect())
    #连接ap
    print(sta.connect("wjx_pc","12345678"))
    #查看状态
    print(sta.status())
    

def ap_test():
    ap=network.WLAN(network.AP_IF)
    #查看ap是否激活
    print(ap.active())
    #配置并创建ap
    ap.config(ssid='k230_ap_wjx', channel=11, key='12345678')
    #查看ap的ssid号
    print(ap.config('ssid'))
    #查看ap的channel
    print(ap.config('channel'))
    #查看ap的所有配置
    print(ap.config())
    #查看ap的状态
    print(ap.status())
    #sta是否连接ap
    print(ap.isconnected())

sta_test()
#ap_test()

