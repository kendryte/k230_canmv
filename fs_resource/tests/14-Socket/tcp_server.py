#配置 tcp/udp socket调试工具
import socket
import network
import time


CONTENT = b"""
Hello #%d from k230 canmv MicroPython!
"""



def server():
    #获取lan接口
    a=network.LAN()
    if(a.active()):
        a.active(0)
    a.active(1)
    a.ifconfig("dhcp")
    ip = a.ifconfig()[0]
    print(a.ifconfig())
    
    counter=1
    
    #建立socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
    #设置属性
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    #获取地址及端口号 对应地址
    ai = socket.getaddrinfo(ip, 8080)
    print("Address infos:", ai,8080)
    addr = ai[0][-1]
    print("Connect address:", addr)
    #绑定
    s.bind(addr)
    #监听
    s.listen(5)
    print("tcp server %s port:%d\n" % ((network.LAN().ifconfig()[0]),8080))


    while True:
        #接受连接
        res = s.accept()
        client_sock = res[0]
        client_addr = res[1]
        print("Client address:", client_addr)
        print("Client socket:", client_sock)
        client_sock.setblocking(False)

        client_stream = client_sock
        #发送字符传
        client_stream.write(CONTENT % counter)

        while True:
            #读取内容
            h = client_stream.read()
            if h != b"" :
                print(h)
                #回复内容
                client_stream.write("recv :%s" % h)

            if "end" in h :
                #关闭socket
                client_stream.close()
                break

        counter += 1
        if counter > 10 :
            print("server exit!")
            #关闭
            s.close()
            break

#main()
server()
