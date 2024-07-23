import network


def main():
    #获取lan接口
    a=network.LAN()
    #获取网口是否在使用
    print(a.active())
    #关闭网口
    print(a.active(0))
    #使能网口
    print(a.active(1))
    #查看网口 ip，掩码，网关，dns配置
    print(a.ifconfig())
    #设置网口 ip，掩码，网关，dns配置
    print(a.ifconfig(('192.168.0.4', '255.255.255.0', '192.168.0.1', '8.8.8.8')))
    #查看网口 ip，掩码，网关，dns配置
    print(a.ifconfig())
    #设置网口为dhcp模式
    print(a.ifconfig("dhcp"))
    #查看网口 ip，掩码，网关，dns配置
    print(a.ifconfig())
    #查看网口mac地址
    print(a.config("mac"))
    #设置网口mac地址
    print(a.config(mac="42:EA:D0:C2:0D:83"))
    #查看网口mac地址
    print(a.config("mac"))
    #设置网口为dhcp模式
    print(a.ifconfig("dhcp"))
    #查看网口 ip，掩码，网关，dns配置
    print(a.ifconfig())




main()

