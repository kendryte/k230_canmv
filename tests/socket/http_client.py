import socket


def main(use_stream=True):
    #创建socket
    s = socket.socket()
    #获取地址及端口号 对应地址
    ai = socket.getaddrinfo("www.baidu.com", 80)
    #ai = socket.getaddrinfo("10.100.228.5", 8080)

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

