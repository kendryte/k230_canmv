import network
import socket


def main(use_stream=True):
    
    #获取lan接口
    a=network.LAN()
    if(a.active()):
        a.active(0)
    a.active(1)
    a.ifconfig("dhcp")
    
    print(a.ifconfig())
    print(a.config("mac"))
    #创建socket
    s = socket.socket()
    #获取地址及端口号 对应地址
    ai = []
        
    for attempt in range(0, 3):
        try:
            #获取地址及端口号 对应地址
            ai = socket.getaddrinfo("www.baidu.com", 80)
            break
        except:
            print("getaddrinfo again");
    
    if(ai == []):
        print("connet error")
        s.close()
        return

    print("Address infos:", ai)
    addr = ai[0][-1]
    
    print("Connect address:", addr)
    #连接
    s.connect(addr)

    if use_stream:
        # MicroPython socket objects support stream (aka file) interface
        # directly, but the line below is needed for CPython.
        s = s.makefile("rwb", 0)
        #发送http请求
        s.write(b"GET /index.html HTTP/1.0\r\n\r\n")
        #打印请求内容
        print(s.read())
    else:
        #发送http请求
        s.send(b"GET /index.html HTTP/1.0\r\n\r\n")
        #打印请求内容
        print(s.recv(4096))
        #print(s.read())
    #关闭socket
    s.close()


#main()
main(use_stream=True)
main(use_stream=False)