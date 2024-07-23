import network

def ap_test():
    ap=network.WLAN(network.AP_IF)
    if(ap.active() == False):
        ap.active(1)
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

ap_test()

