#配置 tcp/udp socket调试工具
import network
import socket
import time

def client():
    #获取lan接口
    a=network.LAN()
    if(a.active()):
        a.active(0)
    a.active(1)
    a.ifconfig("dhcp")
    print(a.ifconfig())
    
    #建立socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    #获取地址及端口号 对应地址
    ai = socket.getaddrinfo("172.16.1.174", 8080)
    #ai = socket.getaddrinfo("10.10.1.94", PORT)
    print("Address infos:", ai)
    addr = ai[0][-1]

    print("Connect address:", addr)
    #连接地址
    if(s.connect(addr) == False):
        s.close()
        print("conner err")
        return

    for i in range(10):
        str="K230 tcp client send test {0} \r\n".format(i)
        print(str)
        #print(s.send(str))
        #发送字符串
        print(s.write(str))
        time.sleep(0.2)
        #time.sleep(1)
        #print(s.recv(4096))
        #print(s.read())
    #延时1秒
    time.sleep(1)
    #关闭socket
    s.close()
    print("end")



#main()
client()


