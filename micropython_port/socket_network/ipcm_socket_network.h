/* Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __IPCM_SOCKET__H__
#define __IPCM_SOCKET__H__

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netdb.h>
#include "k_type.h"
#include "k_ipcmsg.h"
#include "py/runtime.h"
#include <poll.h>

#if defined(__cplusplus)
extern "C" {
#endif

#pragma pack(push)
#pragma pack(1)



typedef struct st_mcsk_connect_s{
    int sockfd;
    struct sockaddr addr;
    socklen_t addrlen;
}mcsk_connect_t;

enum {
    GENADDINFO_TYPE_NODE = 255,
    GENADDINFO_TYPE_SERVICE = 254,
    GENADDINFO_TYPE_ADDINFO_FLAGES=1,
    GENADDINFO_TYPE_ADDINFO_FAMILY = 2,
    GENADDINFO_TYPE_ADDINFO_SOCTYPE = 3,
    GENADDINFO_TYPE_ADDINFO_PROTOC = 4,
    GENADDINFO_TYPE_ADDINFO_ADDLEN = 5,
    GENADDINFO_TYPE_ADDINFO_ADD = 6,
    GENADDINFO_TYPE_ADDINFO_CANONAME = 7,
    GENADDINFO_TYPE_ADDINFO_NEXT = 8,
};    



#pragma pack(pop)

enum en_mcsk_func__{
    FUNC_SOCKT=1,
    FUNC_READ,
    FUNC_WRITE,
    FUNC_CONNECT,
    FUNC_BIND,
    FUNC_LISTEN,
    FUNC_ACCEPT,
    FUNC_RECV,
    FUNC_RECVFROM,
    FUNC_SEND,
    FUNC_SENDTO,
    FUNC_SETSOCKTOPT,
    FUNC_FCNTL,
    FUNC_CLOSE,
    FUNC_GETADDRINFO,
    FUNC_NETWORK_CMD,
    FUNC_POLL,
};

int ipcm_socket(int domain, int type, int protocol);
ssize_t ipcm_read(int fd, void *buf, size_t count);
ssize_t ipcm_write(int fd, const void *buf, size_t count);
int ipcm_connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int ipcm_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int ipcm_listen(int sockfd, int backlog);
int ipcm_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t ipcm_recv(int sockfd, void *buf, size_t len, int flags);
ssize_t ipcm_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t ipcm_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t ipcm_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
int ipcm_setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);                      
int ipcm_fcntl(int fd, int cmd, ... /* arg */ );
int ipcm_close(int fd);

int ipcm_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res);
void ipcm_freeaddrinfo(struct addrinfo *res);                    
int ipcm_socket_server_init(void);     

int ipcm_network_exe_cmd(char *cmd, char *buf, int len);    
mp_obj_t ipcm_network_lan_ifconfig(char *netname,  size_t n_args, const mp_obj_t *args);
int ipcm_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#define IPCM_MAX_SIZE (1024-0x50)


#if defined(__cplusplus)
}
#endif

#endif 