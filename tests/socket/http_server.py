# port from micropython/examples/network/http_server.py
import socket
import network
import time
# print(network.LAN().ifconfig()[0])
# print("Listening, connect your browser to http://%s:8081/" % (network.LAN().ifconfig()[0]))

CONTENT = b"""\
HTTP/1.0 200 OK

Hello #%d from k230 canmv MicroPython!
"""


def main(micropython_optimize=True):
    #建立socket
    s = socket.socket()
    #获取地址及端口号 对应地址
    # Binding to all interfaces - server will be accessible to other hosts!
    ai = socket.getaddrinfo("0.0.0.0", 8081)
    print("Bind address info:", ai)
    addr = ai[0][-1]
    #设置属性
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    #绑定地址
    s.bind(addr)
    #开始监听
    s.listen(5)
    print("Listening, connect your browser to http://%s:8081/" % (network.LAN().ifconfig()[0]))

    counter = 0
    while True:
        #接受连接
        res = s.accept()
        client_sock = res[0]
        client_addr = res[1]
        print("Client address:", client_addr)
        print("Client socket:", client_sock)
        #非阻塞模式
        client_sock.setblocking(False)
        if not micropython_optimize:
            # To read line-oriented protocol (like HTTP) from a socket (and
            # avoid short read problem), it must be wrapped in a stream (aka
            # file-like) object. That's how you do it in CPython:
            client_stream = client_sock.makefile("rwb")
        else:
            # .. but MicroPython socket objects support stream interface
            # directly, so calling .makefile() method is not required. If
            # you develop application which will run only on MicroPython,
            # especially on a resource-constrained embedded device, you
            # may take this shortcut to save resources.
            client_stream = client_sock

        print("Request:")
        #获取内容
        req = client_stream.read()
        print(req)
        
        while True:
            ##获取内容
            h = client_stream.read()            
            if h == b"" or h == b"\r\n":
                break
            print(h)
        #回复内容
        client_stream.write(CONTENT % counter)
        #time.sleep(0.5)
        #关闭
        client_stream.close()
        # if not micropython_optimize:
        #     client_sock.close()
        counter += 1
        #print("wjx", counter)
        if counter > 20 :
            print("http server exit!")
            #关闭 
            s.close()
            break


main()
