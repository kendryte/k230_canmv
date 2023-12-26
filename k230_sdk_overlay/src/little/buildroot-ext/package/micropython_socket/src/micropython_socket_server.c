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
#include <sys/prctl.h>
#include <stdio.h>
#include <pthread.h>
#include "k_type.h"
#include "k_ipcmsg.h"
#include "micropython_socket_server.h"
#include <unistd.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>


#define MIC_SOCK_SERVR_NAME "socket_micropython"

//#include "sample_define.h"
#define SEND_SYNC_MODULE_ID             1
#define SNED_ASYNC_MODULE_ID            2
#define SEND_ONLY_MODULE_ID             3

void dump_gdinfo(struct addrinfo *pa)
{
    printf(" f=%x %x %x %x adlen %x \n", pa->ai_flags,pa->ai_family, pa->ai_socktype, pa->ai_protocol, pa->ai_addrlen);
    if(pa->ai_addr)
        printf("aiadd =%x \n", *(int *)pa->ai_addr);

    if(pa->ai_canonname)
        printf("ai_canonname  %s", pa->ai_canonname);

    printf("next=%p\n",pa->ai_next );

}

static void dump_buf(char *buf, int len, char *prompt)
{
    int i=0;
    printf("%s dump %p 0x%x", prompt, buf, len);
    if(buf == NULL)
        return;
    for(i = 0;i < len; i++){
        if(i%32 == 0)
            printf("\n%p", (void*)buf+i); //add
        printf(" %02hx", *(buf+i));
    }
    printf("\n");

}
static char * tlv_add_data(char *p, int len, unsigned short t,unsigned short l,const char *value)
{
    //printf("%s len=%x need%x type %x\n", __func__, len, l, t);

    if(len < (l +4)){
        printf("%s len=%x %x %x\n", __func__, len, l, t);
    }
    assert(len >= (l +4));

    memcpy(p, &t, sizeof(t));
    p=p+sizeof(t);
    memcpy(p, &l, sizeof(l));
    p=p+sizeof(l);
    memcpy(p, value, l);
    p=p+l;
    return p;
}
/*
const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res
                       */
static void *tlv_get_data(char *pd, int dlen, unsigned short type, int *outlen)
{
    char *data=NULL;
    int i = 0;

    unsigned short *t;
    unsigned short *l;
    char  *v;
    for(i = 0; i < dlen; i = i + 4 + *l){
        t=(unsigned short *)(pd+i);
        l=(unsigned short *)(pd+i+2);
        v=(char *)(pd+i+4);
        //printf("t l v %x %x,add  %p %p %p\n", *t, *l, t,l,v );
        if(*t == type){
            data = v;
            if(outlen)
                *outlen = *l;
            break;
        }
    }
    return data;
}
int tlv_getaddinfo(char *pd, int dlen, int index,struct addrinfo *addinfo)
{
    unsigned short type=0;
    void *value = NULL;
    memset(addinfo, 0, sizeof(*addinfo));

    type =  GENADDINFO_TYPE_ADDINFO_FLAGES + (index<<8);
    value = tlv_get_data(pd, dlen, type++,NULL);
    if(value == NULL)
        return -1;    
    addinfo->ai_flags =*(int*)value;

    value = tlv_get_data(pd, dlen, type++,NULL);
    if(value)
        addinfo->ai_family =*(int*)value;

    value = tlv_get_data(pd, dlen, type++,NULL);
    if(value)
        addinfo->ai_socktype =*(int*)value;

    value = tlv_get_data(pd, dlen, type++,NULL);
    if(value)
        addinfo->ai_protocol =*(int*)value;

    value = tlv_get_data(pd, dlen, type++,NULL); //ai_addrlen
    if(value == NULL)
        return -1;
    memcpy(&addinfo->ai_addrlen, value, sizeof(addinfo->ai_addrlen));

    if(addinfo->ai_addrlen == 0)
        addinfo->ai_addr = NULL;
    else 
        addinfo->ai_addr = tlv_get_data(pd, dlen, type,NULL);

    type++;

    addinfo->ai_canonname = tlv_get_data(pd, dlen, type++,NULL);
    //value = tlv_get_data(pd, dlen, type++,NULL);
    addinfo->ai_next = tlv_get_data(pd, dlen, type++,NULL);
    return 0;
}
static char * tlv_add_addrinfo_s(char *p, int len, int index, const struct addrinfo *hints)
{
    char next= 1;
    char *pt = p;
    unsigned short type = (index<<8) + GENADDINFO_TYPE_ADDINFO_FLAGES ;
    
    if(hints){
        pt = tlv_add_data(pt, len-(pt-p), type++, sizeof(hints->ai_flags), (const char*)&hints->ai_flags);
        pt = tlv_add_data(pt, len-(pt-p), type++, sizeof(hints->ai_family), (const char*)&hints->ai_family);
        pt = tlv_add_data(pt, len-(pt-p), type++, sizeof(hints->ai_socktype), (const char*)&hints->ai_socktype);
        pt = tlv_add_data(pt, len-(pt-p), type++, sizeof(hints->ai_protocol), (const char*)&hints->ai_protocol);
        pt = tlv_add_data(pt, len-(pt-p), type++, sizeof(hints->ai_addrlen), (const char*)&hints->ai_addrlen);
        
        if(hints->ai_addr)
            pt = tlv_add_data(pt, len-(pt-p), type, hints->ai_addrlen, (const char*)hints->ai_addr);//6

        

        if(hints->ai_canonname != NULL )
            pt = tlv_add_data(pt, len-(pt-p), type =(index<<8) + GENADDINFO_TYPE_ADDINFO_CANONAME, \
                                strlen(hints->ai_canonname)+1, (const char*)hints->ai_canonname);
      
        if(hints->ai_next)
             pt = tlv_add_data(pt, len-(pt-p), type =(index<<8) + GENADDINFO_TYPE_ADDINFO_NEXT, sizeof(next), (const char*)&next);
    }
    return pt;
}

void* rcv_thread(void* arg)
{
    prctl(PR_SET_NAME, "rcv_thread", 0, 0, 0);
    k_s32* s32Id = (k_s32*)arg;
    //printf("start receive msg  from %d\n", *s32Id);
    kd_ipcmsg_run(*s32Id);
    return NULL;
}
static int mcsk_srv_proc_socket(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    
    int fd = 0;
    int domain =msg->as32PrivData[0] ;     
    int type = msg->as32PrivData[1] ;
    int protocol = msg->as32PrivData[2];

    fd = socket( domain,  type,  protocol);
    
    *len = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    return fd;
}

static int mcsk_srv_proc_bind(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    /* int bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen); */
    int ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    const struct sockaddr *addr  = (struct sockaddr*)msg->pBody;//msg->as32PrivData[1];
    int addrlen = msg->as32PrivData[2];
 
    ret = bind( sockfd,  addr,  addrlen);
    *len = 0;
    return ret;
}
/*int connect(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen);
*/
static int mcsk_srv_proc_connect(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    
    int ret = 0;
    mcsk_connect_t *s = (mcsk_connect_t *)msg->pBody;
    ret = connect(s->sockfd, &s->addr, s->addrlen);
    //printf("mcsk_srv_proc_connect %x\n", ret);
    *len = 0;
    return ret;
}

int mcsk_srv_proc_getaddinfo_fill_resp_body(struct addrinfo *res, char *body, int *len)
{
    struct addrinfo *pcur =  res;
    char *pb = body;
    int index = 1;
    int have_add_index = 1;
    while(pcur){
        //printf("index =%d  %p %ld %x %p %p\n",index,pb, *len - (pb-body), *(int*)pcur->ai_addr, pcur, pcur->ai_next);
        //dump_gdinfo(pcur);
        if((index < 10) || (pcur->ai_next == NULL)){
            pb = tlv_add_addrinfo_s(pb, *len - (pb-body), have_add_index, pcur);
            have_add_index++;
        }
            
        pcur = pcur->ai_next;
        index++;        
    }
    *len = pb-body;
    return 0;
}

static int mcsk_srv_proc_getaddinfo(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    int ret;
    
    const char *node;
    const char *service;
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    //printf("f=%s l=%d\n", __func__,__LINE__);

    node = tlv_get_data(msg->pBody, msg->u32BodyLen, GENADDINFO_TYPE_NODE,NULL);
    service = tlv_get_data(msg->pBody, msg->u32BodyLen, GENADDINFO_TYPE_SERVICE,NULL);
    tlv_getaddinfo(msg->pBody, msg->u32BodyLen, 1, &hints);
    //printf("node=%s %s %p \n", node, service, res);
    //dump_gdinfo(&hints);
    
    ret = getaddrinfo(node, service, &hints, &res);
    //printf("node=%s %s %p \n", node, service, res);
    
    if(ret == 0){
        mcsk_srv_proc_getaddinfo_fill_resp_body(res, body, len);
        freeaddrinfo(res);
    }
    
    return ret;
}
ssize_t mcsk_srv_proc_send(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    void *buf = msg->pBody;
    size_t len = msg->u32BodyLen;
    int flags = msg->as32PrivData[3];

    //ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    ret = send(sockfd, buf, len,flags);
    *bdlen = 0;
    return ret;
}

ssize_t mcsk_srv_proc_write(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int fd = msg->as32PrivData[0];
    void *buf = msg->pBody;
    size_t count = msg->as32PrivData[2];
    assert(count == msg->u32BodyLen);

    ret = write(fd, buf, count);
    *bdlen = 0;
    return ret;
}

ssize_t mcsk_srv_proc_recv(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    size_t expace_len = msg->as32PrivData[2];
    int flags = msg->as32PrivData[3];
    if(expace_len > *bdlen){
        printf("error  %lx %x\n", expace_len, *bdlen);
    }

    //ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    ret = recv(sockfd, body, expace_len,flags);
    if(ret >= 0)
        *bdlen = ret;
    return ret;
}
ssize_t mcsk_srv_proc_read(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
   
    int sockfd = msg->as32PrivData[0];
    size_t expace_len = msg->as32PrivData[2];

    //printf("f=%s l=%d %x \n", __func__,__LINE__, expace_len);

    //int flags = msg->as32PrivData[3];
    if(expace_len > *bdlen){
        printf("error  %lx %x\n", expace_len, *bdlen);
    }
    ret = read(sockfd, body, expace_len);
    
    
    if(ret >= 0)
        *bdlen = ret; 
    else{
        *bdlen = 0;
        if(errno == EAGAIN)
            ret = 0;
    }
    //printf("f=%s l=%d %x %x %s \n", __func__,__LINE__, ret , errno, strerror(errno));
    return ret;
}

static int mcsk_srv_proc_close(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    int ret = 0;
    int fd = msg->as32PrivData[0];
    printf("f=%s l=%d %x \n", __func__,__LINE__, fd);
    //int close(int fd);
    ret = close(fd);
    *len = 0;
    return ret;
}
//int ipcm_poll(struct pollfd *fds, nfds_t nfds, int timeout)
static int mcsk_srv_proc_poll(k_ipcmsg_message_t* msg, char  *body, int *len)
{
    int ret = 0;
    const  nfds_t nfds = msg->as32PrivData[0];
    int timeout = msg->as32PrivData[1];
    struct pollfd fds;

    //printf("f=%s l=%d %x\n", __func__, __LINE__, nfds);
    
    assert(sizeof(struct pollfd) <= msg->u32BodyLen);
    memcpy(&fds, msg->pBody, sizeof( struct pollfd));
    ret = poll(&fds, nfds, timeout);
    assert(*len >= sizeof(struct pollfd));
    *len = 0;
    if(ret > 0){
        memcpy(body, &fds, sizeof(struct pollfd));
        *len = sizeof(struct pollfd);
    }
        
    //printf("f=%s l=%d %x\n", __func__, __LINE__, ret, fds.revents);
    return ret;
}
ssize_t mcsk_srv_proc_setsocktopt(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    int level  = msg->as32PrivData[1];
    int optname = msg->as32PrivData[2];
    const void *optval = msg->pBody;
    socklen_t optlen = msg->as32PrivData[4];
    
    /*     int setsockopt(int sockfd, int level, int optname,
                    const void *optval, socklen_t optlen);    */
    ret = setsockopt( sockfd,  level,  optname, optval,  optlen);
    
    *bdlen = 0; 
    return ret;
}
//int listen(int sockfd, int backlog);
ssize_t mcsk_srv_proc_listen(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    int backlog  = msg->as32PrivData[1];

    ret = listen( sockfd, backlog);
    
    *bdlen = 0; 
    return ret;
}
ssize_t mcsk_srv_proc_accept(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    
    int sockfd = msg->as32PrivData[0];
    struct sockaddr addr;
    socklen_t  addrlen =0;
    assert(*bdlen >= msg->as32PrivData[2]);
    //int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    ret = accept( sockfd, &addr, &addrlen);
    *bdlen = addrlen;
    if(addrlen > 0)
        memcpy(body, &addr, addrlen);
    //dump_buf(body,addrlen, "accept" );

    return ret;
}
/*
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
*/
ssize_t mcsk_srv_proc_recvfrom(k_ipcmsg_message_t* msg, char  *body, int *bdlen, struct sockaddr *src_addr, socklen_t *addrlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d %x %x \n", __func__,__LINE__, ret, *addrlen);
    int sockfd = msg->as32PrivData[0];
    size_t expace_len = msg->as32PrivData[1];
    int flags = msg->as32PrivData[2];
    
    if(expace_len > *bdlen){
        printf("error  %lx %x\n", expace_len, *bdlen);
        assert(expace_len < *bdlen);
    }
    
    ret = recvfrom( sockfd, body, expace_len, flags, src_addr, addrlen);
    if(ret >= 0)
        *bdlen = ret;
    //dump_buf(src_addr, *addrlen, "d");
    //printf("f=%s l=%d %x %x \n", __func__,__LINE__, ret, *addrlen);
    return ret;
}

ssize_t mcsk_srv_proc_sendto(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int sockfd = msg->as32PrivData[0];
    int flags = msg->as32PrivData[1];
    socklen_t addrlen = msg->as32PrivData[2];
    struct sockaddr dest_addr;
    if(addrlen)
        memcpy(&dest_addr,  &msg->as32PrivData[3],  addrlen);

    /*ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);*/
    ret = sendto( sockfd, msg->pBody, msg->u32BodyLen, flags, &dest_addr, addrlen);
    *bdlen = 0; 
    return ret;
}
ssize_t mcsk_srv_proc_fcntl(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    //printf("f=%s l=%d\n", __func__,__LINE__);
    int fd = msg->as32PrivData[0];
    int cmd = msg->as32PrivData[1];
    int cmd_param = msg->as32PrivData[2];
   
    //int fcntl(int fd, int cmd, ... /* arg */ );
    ret = fcntl( fd, cmd, cmd_param);
    *bdlen = 0; 
    return ret;
}
ssize_t mcsk_srv_proc_netcmd(k_ipcmsg_message_t* msg, char  *body, int *bdlen)
{
    ssize_t ret = 0;
    FILE * p_file = NULL;
    char *tmp;
    int pos=0;

    char *cmd=msg->pBody;
    
    //printf("f =%s l=%d cmd=%s ret =%d\n", __func__, __LINE__, cmd, ret);

    if(msg->as32PrivData[0] > 0 ){ //need cmd contect;
        if(body == NULL)
            return -2;
        body[0]=0;
        p_file = popen(cmd, "r");
        if (!p_file) {  
            fprintf(stderr, "Erro to popen %x ,%s ", errno,strerror(errno));  
            *bdlen = 0;
            return -1;
        }  

        
        do{
            tmp=fgets(body+pos, *bdlen-pos, p_file);
            if(tmp){
                pos += strlen(tmp);
                //printf("pos=%d tmp=%s \n", pos, tmp);
            }
        }while((tmp) && (*bdlen > pos));

        //printf("l=%d %s %lx p=%x %x %x %x\n", strlen(body), body, p_file, pos, body[pos],body[pos-1],body[pos-2]);
    
        for( ;  (pos > 0) &&
                ( (body[pos-1] == '\n')  || (body[pos-1] == '\r') || (body[pos-1] == ' ') || (body[pos-1] == '\t') );
                pos--
             )
            body[pos - 1] = 0;
            
        //printf("l=%d %s %lx p=%x %x %x %x\n", strlen(body), body, p_file, pos, body[pos],body[pos-1],body[pos-2]);
        ret = pclose(p_file);   
        *bdlen = strlen(body)+1;  
    }else  { //only ret 
        ret = system(cmd);
        *bdlen = 0;
    }
    //printf("f =%s l=%d cmd=%s ret =%d\n", __func__, __LINE__, cmd, ret);
    return ret;
}
static void mcsk_rcv_msg_proc(k_s32 s32Id, k_ipcmsg_message_t* msg)
{
    k_s32 s32Ret = 0;
    char body[1025];
    
    int bd_len = sizeof(body);
    k_ipcmsg_message_t *respMsg = NULL;

    struct sockaddr src_addr;
    socklen_t addrle = sizeof(src_addr);

    //printf("recv--->>server recv id %x cmd %x len=%d \n", s32Id, msg->u32CMD, msg->u32BodyLen);
    //dump_buf(msg->pBody, msg->u32BodyLen, "s rsv ");
    

    switch (msg->u32CMD){
        case FUNC_SOCKT:        s32Ret = mcsk_srv_proc_socket(msg, body, &bd_len);break; //
        case FUNC_READ:         s32Ret = mcsk_srv_proc_read(msg, body, &bd_len);break;
        case FUNC_WRITE:        s32Ret = mcsk_srv_proc_write(msg, body, &bd_len);break;
        case FUNC_CONNECT:      s32Ret = mcsk_srv_proc_connect(msg, body, &bd_len);break;
        case FUNC_BIND:         s32Ret = mcsk_srv_proc_bind(msg, body, &bd_len);break;
        case FUNC_LISTEN:       s32Ret = mcsk_srv_proc_listen(msg, body, &bd_len);break;
        case FUNC_ACCEPT:       s32Ret = mcsk_srv_proc_accept(msg, body, &bd_len);break;
        case FUNC_RECV:         s32Ret = mcsk_srv_proc_recv(msg, body, &bd_len);break;
        case FUNC_RECVFROM:     s32Ret = mcsk_srv_proc_recvfrom(msg, body, &bd_len, &src_addr, &addrle);break;
        case FUNC_SEND:         s32Ret = mcsk_srv_proc_send(msg, body, &bd_len);break;
        case FUNC_SENDTO:       s32Ret = mcsk_srv_proc_sendto(msg, body, &bd_len);break;
        case FUNC_SETSOCKTOPT:  s32Ret = mcsk_srv_proc_setsocktopt(msg, body, &bd_len);break;
        case FUNC_FCNTL:        s32Ret = mcsk_srv_proc_fcntl(msg, body, &bd_len);break;
        case FUNC_CLOSE:        s32Ret = mcsk_srv_proc_close(msg, body, &bd_len);break;
        case FUNC_GETADDRINFO:  s32Ret = mcsk_srv_proc_getaddinfo(msg, body, &bd_len);break;
        case FUNC_NETWORK_CMD:  s32Ret = mcsk_srv_proc_netcmd(msg, body, &bd_len);break;
        case FUNC_POLL:         s32Ret = mcsk_srv_proc_poll(msg, body, &bd_len);break;

        default: dump_buf((char*)msg, sizeof(*msg),"recv error msg\n");break;
    }
   
    //system("free");
    respMsg = kd_ipcmsg_create_resp_message(msg, s32Ret, body, bd_len);    
    //printf("f=%s l=%d %p %d\n", __func__, __LINE__, body, bd_len);
    if(respMsg == NULL){
        printf("errro respmsg is NULL \n");
        s32Ret = -1;
        respMsg = kd_ipcmsg_create_resp_message(msg, s32Ret, body, 0);
    }
    if(s32Ret == -1)  {
        respMsg->as32PrivData[0] = errno;        
        printf("f=%s l=%d errno=%x %s ,%x \n", __func__, __LINE__, errno, strerror(errno), msg->u32CMD);
    }  

    if(msg->u32CMD == FUNC_RECVFROM){
        assert(6*sizeof(respMsg->as32PrivData[0]) >= addrle);
        respMsg->as32PrivData[1] = addrle;
        memcpy(&respMsg->as32PrivData[2], &src_addr, addrle);
    }

        
    //printf("f=%s l=%d\n", __func__, __LINE__);
    kd_ipcmsg_send_async(s32Id, respMsg, NULL);
    //dump_buf(respMsg->pBody, respMsg->u32BodyLen, "s send ");
    //printf("f=%s l=%d\n", __func__, __LINE__);
    kd_ipcmsg_destroy_message(respMsg);
    //system("free");
    //printf("end<<---server proc end cmd  %x ret %x\n",  msg->u32CMD, s32Ret);     
}


int main(int argc, char** argv)
{

    k_s32 s32Id1;
    pthread_t threadid1;
    int ret = 0;
    //k_char cmd[64];
    k_ipcmsg_connect_t stConnectAttr={1,400,1};

  
    ret = kd_ipcmsg_add_service(MIC_SOCK_SERVR_NAME, &stConnectAttr);
    if(ret != 0){
        printf("kd_ipcmsg_add_service return err:%x\n", ret);
        return -1;
    }

    if (0 != kd_ipcmsg_connect(&s32Id1, MIC_SOCK_SERVR_NAME, mcsk_rcv_msg_proc)){
        printf("Connect fail\n");
        return -1;
    }

    if (0 != pthread_create(&threadid1, NULL, rcv_thread, &s32Id1)){
        printf("pthread_create rcv_thread fail\n");
        return -2;
    }

    // while (0 != strncmp(fgets(cmd, 64, stdin), "q", 1))
    // {
    //     printf("Press q to quit\n");
    // }
    while(1)
    sleep(1);

    kd_ipcmsg_disconnect(s32Id1);
    pthread_join(threadid1, NULL);
    kd_ipcmsg_del_service(MIC_SOCK_SERVR_NAME);
    printf("quit\n");

    return K_SUCCESS;
}



