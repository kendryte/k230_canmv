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
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "k_type.h"
#include "k_ipcmsg.h"
//#include "sample_define.h"
#include "ipcm_socket_network.h"
#include <assert.h>
#include <stdarg.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "extmod/modnetwork.h"
#include <poll.h>

#define MIC_SOCK_SERVR_NAME "socket_micropython" 

k_s32 g_ipcm_socket_clinet_s32Id = 0;
static pthread_t receivePid0 = 0;
// void dump_gdinfo(const struct addrinfo *pa)
// {
//     printf(" f=%x %x %x %x adlen %x \n", pa->ai_flags,pa->ai_family, pa->ai_socktype, pa->ai_protocol, pa->ai_addrlen);
//     if(pa->ai_addr)
//         printf("aiadd =%x \n", *(int *)pa->ai_addr);

//     if(pa->ai_canonname)
//         printf("ai_canonname  %s", pa->ai_canonname);

//     printf("next=%p\n",pa->ai_next );

// }

void dump_buf(char *buf, int len, char *prompt)
{
    int i=0;
    fprintf(stderr,"%s dump %p 0x%x", prompt, buf, len);
    if(buf==NULL)
        return;
    for(i = 0;i < len; i++){
        if(i%32 == 0)
            fprintf(stderr,"\n%p", buf+i); //add
        fprintf(stderr," %02hx", *(buf+i));
    }
    fprintf(stderr,"\n");
}
static char * tlv_add_data(char *p, int len, unsigned short t,unsigned short l,const char *value)
{
    //printf("%s len=%x %x %x\n", __func__, len, l, t);

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
static char * tlv_add_addrinfo(char *p, int len, int index, const struct addrinfo *hints)
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
        if(*t == type){
            data = v;
            if(outlen)
                *outlen = *l;
            break;
        }
    }
    return data;
}

static int tlv_getaddinfo_c(char *pd, int dlen, int index,struct addrinfo *addinfo)
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
static void mcsk_clinet_rcv_msg_proc(k_s32 s32Id, k_ipcmsg_message_t* msg)
{   //---回复消息不会走到这里；
    printf("xxx  msg form %d:  %s\n", s32Id, (char*)msg->pBody);
    //printf(" msg form %d: %x %x %s\n", s32Id, msg->s32RetVal, msg->bIsResp,(char*)msg->pBody);
}
static void* receive_thread(void* arg)
{
    k_s32* pId = (k_s32*)arg;
    //printf("Run...\n");
    kd_ipcmsg_run(*pId);
    //printf("after Run...\n");
    return NULL;
}

// static int ipcm_socket_uninit()
// {
//     kd_ipcmsg_disconnect(g_ipcm_socket_clinet_s32Id);
//     pthread_join(receivePid0, NULL);
//     kd_ipcmsg_del_service(MIC_SOCK_SERVR_NAME);
//     return 0;
// }

int ipcm_socket_server_init(void)
{
    k_ipcmsg_connect_t stConnectAttr;
    stConnectAttr.u32RemoteId = 0;
    stConnectAttr.u32Port = 400;
    stConnectAttr.u32Priority = 0;

    if(g_ipcm_socket_clinet_s32Id)
       return 0;

    
    kd_ipcmsg_add_service(MIC_SOCK_SERVR_NAME, &stConnectAttr);

    if (0 != kd_ipcmsg_try_connect(&g_ipcm_socket_clinet_s32Id, MIC_SOCK_SERVR_NAME, mcsk_clinet_rcv_msg_proc)){
        printf("Connect fail\n");
        return K_FAILED;
    }
    //printf("%x -----Connect: %d\n", g_ipcm_socket_clinet_s32Id,  kd_ipcmsg_is_connect(g_ipcm_socket_clinet_s32Id));
    if (0 != pthread_create(&receivePid0, NULL, receive_thread, &g_ipcm_socket_clinet_s32Id)){
        printf("pthread_create receive_thread fail\n");
        return K_FAILED;
    }
    return 0;

}

//ok
int ipcm_socket(int domain, int type, int protocol)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    
    //printf("f=%s l=%d\n", __func__,__LINE__);
    ipcm_socket_server_init();
    pReq = kd_ipcmsg_create_message(0, FUNC_SOCKT, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0] = domain;
    pReq->as32PrivData[1] = type;
    pReq->as32PrivData[2] = protocol;
    
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    //printf("ipcm_socket ret =%x %x \n", pResp->s32RetVal,retVal);
    return retVal;
}
ssize_t ipcm_read(int fd, void *buf, size_t count)
{
    ssize_t retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    ipcm_socket_server_init();
    
    printf("f=%s l=%d %lx %lx \n", __func__,__LINE__, clock(), count);
    pReq = kd_ipcmsg_create_message(0, FUNC_READ, buf, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0] = fd;
    pReq->as32PrivData[1] = buf;
    pReq->as32PrivData[2] = count;


    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 200000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        printf("ipcm_read ret =%x %x \n", pResp->s32RetVal,pResp->u32CMD);
        retVal = pResp->s32RetVal;
        if(retVal > 0){
            memcpy(buf, pResp->pBody, retVal);
        }
        kd_ipcmsg_destroy_message(pResp);
    }else {
        printf("f=%s l=%d %lx \n", __func__,__LINE__, retVal);
        retVal = -1;
    }
    return retVal;

}
ssize_t ipcm_write(int fd, const void *buf, size_t count)
{
    ssize_t retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    ipcm_socket_server_init();
    
    //printf("f=%s l=%d\n", __func__,__LINE__);
    pReq = kd_ipcmsg_create_message(0, FUNC_WRITE, buf, count);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=fd;
    pReq->as32PrivData[2]=count;
    //pReq->as32PrivData[3]=flags;

    
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    //printf("f=%s ret =%x %lx count = %lx\n", __func__, pResp->s32RetVal,retVal, count);

    return retVal;
}
//ok
int ipcm_connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    mcsk_connect_t param={sockfd,*addr, addrlen};
    ipcm_socket_server_init();
    //printf("f=%s l=%d\n", __func__,__LINE__);
    pReq = kd_ipcmsg_create_message(0, FUNC_CONNECT, &param, sizeof(param));
    assert(pReq != NULL);
    
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
}
int ipcm_bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen)
{
  
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    ipcm_socket_server_init();

    //printf("f=%s l=%d\n", __func__,__LINE__);
    
    pReq = kd_ipcmsg_create_message(0, FUNC_BIND, addr, addrlen);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[1]=addr;
    pReq->as32PrivData[2]=addrlen;

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
}
int ipcm_listen(int sockfd, int backlog)
{
    //printf("f=%s l=%d\n", __func__,__LINE__);
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    ipcm_socket_server_init();
    //printf("f=%s l=%d\n", __func__,__LINE__);
    
    pReq = kd_ipcmsg_create_message(0, FUNC_LISTEN, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[1]=backlog;
    

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
}
int ipcm_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
   
    
    k_s32 retVal = -1;
    k_s32 len = *addrlen;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    ipcm_socket_server_init();

    //printf("f=%s l=%d\n", __func__,__LINE__);
    
    pReq = kd_ipcmsg_create_message(0, FUNC_ACCEPT, addr, len);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[1]=addr;
    pReq->as32PrivData[2]= len;
    

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 0x7fffffff);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        retVal = pResp->s32RetVal;
        if(len >= pResp->u32BodyLen){
            *addrlen = pResp->u32BodyLen;
            memcpy(addr, pResp->pBody, pResp->u32BodyLen);
            //dump_buf((char*)addr, pResp->u32BodyLen,"d");
        }
        kd_ipcmsg_destroy_message(pResp);
    }else {
        //printf("f=%s l=%d %x \n", __func__,__LINE__, retVal);
        retVal = -1;
    }
    //printf("f=%s  ret =%x %x, len= %x %x \n", __func__, pResp->s32RetVal,retVal, len, pResp->u32BodyLen);
    return retVal;
}
//---ok
ssize_t ipcm_recv(int sockfd, void *buf, size_t len, int flags)
{
    ssize_t retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    ipcm_socket_server_init();
    
    //printf("f=%s l=%d %lx\n", __func__,__LINE__, len);
    pReq = kd_ipcmsg_create_message(0, FUNC_RECV, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[2]=len;
    pReq->as32PrivData[3]=flags;
    
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 0x7fffffff);
    kd_ipcmsg_destroy_message(pReq);    
    //printf("f=%s l=%d %lx\n", __func__,__LINE__, retVal);
    
    if(retVal == 0){
        //printf("ipcm_recv ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        //dump_buf(pResp->pBody, pResp->u32BodyLen, "ipcm_recv");
        memcpy(buf,pResp->pBody, pResp->u32BodyLen );
        retVal = pResp->s32RetVal;
        if(retVal == -1)
            errno = pResp->as32PrivData[0];
        
        //printf("f=%s l=%d %lx %x %x\n", __func__,__LINE__, retVal, pResp->as32PrivData[0],pResp->s32RetVal);
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
}
//OK
ssize_t ipcm_send(int sockfd, const void *buf, size_t len, int flags)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    int have_send=0;
    int cur_need_send = 0;
    int cur_ret = 0;

    ipcm_socket_server_init();

    for( ; have_send < len ;){
        

        cur_need_send = MIN(len - have_send, IPCM_MAX_SIZE);
        //printf("f=%s l=%d %x len %lx %x \n", __func__,__LINE__, cur_need_send, len, have_send);

        pReq = kd_ipcmsg_create_message(0, FUNC_SEND, buf+have_send,  cur_need_send);
        assert(pReq != NULL);
        pReq->as32PrivData[0] = sockfd;
        pReq->as32PrivData[2] = cur_need_send; 
        pReq->as32PrivData[3] = flags;

        retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
        kd_ipcmsg_destroy_message(pReq);    
        
        if(retVal == 0){
            //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
            cur_ret = pResp->s32RetVal;
            kd_ipcmsg_destroy_message(pResp);

            if( cur_ret < 0){
                retVal = cur_ret;
                break;
            }
            if( cur_ret < cur_need_send){
                have_send += cur_ret;
                retVal =  have_send;
                break;
            }
            have_send += cur_ret;
            retVal = have_send;  
            
        }else {
            retVal = -1;
            break;
        }
    
    }
  
    return retVal;
}
ssize_t ipcm_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen)
{
    ssize_t retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    socklen_t inlen = *addrlen;
    
    ipcm_socket_server_init();
    //printf("f=%s l=%d %lx\n", __func__,__LINE__, len);
    pReq = kd_ipcmsg_create_message(0, FUNC_RECVFROM, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[1]=len;
    pReq->as32PrivData[2]=flags;

    
    //printf("l=%x %lx\n", *addrlen, sizeof(pReq->as32PrivData));

    //printf("f=%s l=%d %lx\n", __func__,__LINE__, len);
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 0x7fffffff);
    kd_ipcmsg_destroy_message(pReq);    
    //printf("f=%s l=%d %lx %lx\n", __func__,__LINE__, len, retVal);
    if(retVal == 0){
        //printf("ipcm_recvfrom ret =%x %x %x\n", pResp->s32RetVal,inlen, pResp->as32PrivData[0]);
        //dump_buf(pResp->pBody, pResp->u32BodyLen, "ipcm_recv");
        memcpy(buf,pResp->pBody, pResp->u32BodyLen );
        retVal = pResp->s32RetVal;
        assert(inlen >= pResp->as32PrivData[1]);
        if(inlen >= pResp->as32PrivData[1] ){
            *addrlen = pResp->as32PrivData[1];
            memcpy(src_addr, &pResp->as32PrivData[2], pResp->as32PrivData[1]);
            //dump_buf((char*)src_addr, pResp->as32PrivData[0], "addrwjx");
        }        
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    //printf("f=%s l=%d %lx ret %lx \n", __func__,__LINE__, len, retVal);
    return retVal;
}
ssize_t ipcm_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    int have_send=0;
    int cur_need_send = 0;
    int cur_ret = 0;

    
    ipcm_socket_server_init();
    while(have_send < len){
        cur_need_send = MIN(len - have_send, IPCM_MAX_SIZE);

        pReq = kd_ipcmsg_create_message(0, FUNC_SENDTO,  buf + have_send,  cur_need_send);
        assert(pReq != NULL);
        pReq->as32PrivData[0]=sockfd;
        pReq->as32PrivData[1]=flags;
        pReq->as32PrivData[2]=addrlen;
        if(addrlen)
            memcpy(&pReq->as32PrivData[3], dest_addr, addrlen);

        retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
        kd_ipcmsg_destroy_message(pReq);    
        
        if(retVal == 0){
            //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
            cur_ret = pResp->s32RetVal;
            kd_ipcmsg_destroy_message(pResp);
            if( cur_ret < 0){
                retVal = cur_ret;
                break;
            }
            if( cur_ret < cur_need_send){
                have_send += cur_ret;
                retVal =  have_send;
                break;
            }
            have_send += cur_ret;
            retVal = have_send;  
        }else {
            retVal = -1;
            break;
        }
    }
    
    return retVal;
}
int ipcm_setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen)
{
    /*
    int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);
    */
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    ipcm_socket_server_init();
    //printf("f=%s l=%d\n", __func__,__LINE__);
    
    pReq = kd_ipcmsg_create_message(0, FUNC_SETSOCKTOPT, optval, optlen);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=sockfd;
    pReq->as32PrivData[1]=level;
    pReq->as32PrivData[2]=optname;
    pReq->as32PrivData[3]=optval;
    pReq->as32PrivData[4]=optlen;
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        //printf("f=%s l=%d %x\n", __func__,__LINE__, retVal);
        retVal = -1;
    }
    return retVal;
}
int ipcm_fcntl(int fd, int cmd, ... /* arg */ )
{
    //int fcntl(int fd, int cmd, ... /* arg */ );

    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    int cmd_param; 
    va_list args;

    va_start(args, cmd);
    cmd_param = va_arg(args, int);
    va_end(args);
    //printf("f=%s l=%d\n", __func__,__LINE__);
    
    ipcm_socket_server_init();
    pReq = kd_ipcmsg_create_message(0, FUNC_FCNTL, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=fd;
    pReq->as32PrivData[1]=cmd;
    pReq->as32PrivData[2]=cmd_param;

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_fcntl ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
        //printf("ipcm_fcntl retX =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, retVal);
    }
    return retVal;
}
int ipcm_close(int fd)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    //printf("f=%s l=%d %x\n", __func__,__LINE__,fd);
    
    ipcm_socket_server_init();
    pReq = kd_ipcmsg_create_message(0, FUNC_CLOSE, NULL, 0);
    assert(pReq != NULL);
    pReq->as32PrivData[0]=fd;

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
    return 0; 

}
int ipcm_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    //printf("f=%s l=%d %x\n", __func__,__LINE__,fd);
    
    ipcm_socket_server_init();
    pReq = kd_ipcmsg_create_message(0, FUNC_POLL, fds, sizeof( struct pollfd));
    assert(pReq != NULL);
    pReq->as32PrivData[0] = nfds;
    pReq->as32PrivData[1] = timeout;

    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        if(pResp->s32RetVal > 0){
            assert(pResp->u32BodyLen <= sizeof(struct pollfd));
            memcpy(fds, pResp->pBody, sizeof(struct pollfd));
            //dump_buf((char *)fds, pResp->u32BodyLen, "d");
            //printf("fds=%x\n", fds->revents);
        }
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;

}



static char *ipcm_getaddrinfo_build_body(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res, char *body, int *len)
{
    char *p=body;
    int addrinfo_index = 1;

    p = tlv_add_data(p, (*len)-(p-body), GENADDINFO_TYPE_NODE, strlen(node)+1, node);
    p = tlv_add_data(p, (*len)-(p-body), GENADDINFO_TYPE_SERVICE, strlen(service)+1, service);
    if(hints){
       p=tlv_add_addrinfo(p, (*len)-(p-body), addrinfo_index++, hints);
    }
    *len = (p-body);
    return body;
}
void ipcm_freeaddrinfo(struct addrinfo *res)
{
    struct addrinfo  *p= res;
    struct addrinfo  *pn= NULL;
    for(;p; p=pn){
        if(p->ai_addr) 
            free(p->ai_addr);
        if(p->ai_canonname)
            free(p->ai_canonname);

        pn=p->ai_next;
        free(p);
    }
    return;
}
int ipcm_getaddrinfo_parase_respmsg(k_ipcmsg_message_t* pResp, struct addrinfo **res)
{
    int index = 1;
    struct addrinfo a;
    struct addrinfo *pcur = NULL;
    struct addrinfo *palast = NULL;
    int ret;
    while(1){
        memset(&a, 0, sizeof(a));
        ret = tlv_getaddinfo_c(pResp->pBody, pResp->u32BodyLen, index, &a);
        if(ret != 0 )
            break;
        pcur = (struct addrinfo * )malloc(sizeof(struct addrinfo));
        memcpy(pcur, &a, sizeof(struct addrinfo));
        if(a.ai_addr){
            pcur->ai_addr = malloc(a.ai_addrlen);
            memcpy(pcur->ai_addr, a.ai_addr, a.ai_addrlen);
        }
        if(a.ai_canonname){
            pcur->ai_canonname = strdup(a.ai_canonname);
        }
        pcur->ai_next = NULL;

        if(palast)
            palast->ai_next = pcur;
        else 
            *res = pcur;    

        palast = pcur;  
        //printf("f=%s l=%d\n", __func__,__LINE__);

        index++;
    }
    
    return 0;
}
int ipcm_getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res)
{
    ssize_t retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;
    char body[1024]={0};
    int body_len=sizeof(body);
    // printf("ipcm_getaddrinfo %s %s \n", node, service);

    // dump_gdinfo(hints);
    ipcm_socket_server_init();
    
    ipcm_getaddrinfo_build_body(node,service, hints, res, body,&body_len);    
    pReq = kd_ipcmsg_create_message(0, FUNC_GETADDRINFO, body, body_len);  
    assert(pReq != NULL);
    //dump_buf(body, body_len, "c send");  
    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 2000);
    kd_ipcmsg_destroy_message(pReq);   
    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->bIsResp);
        retVal = pResp->s32RetVal;
        ipcm_getaddrinfo_parase_respmsg(pResp, res);
        //dump_buf(pResp->pBody, pResp->u32BodyLen, "c rcv");  
        kd_ipcmsg_destroy_message(pResp);
    }else {
        retVal = -1;
    }
    return retVal;
}


// ifconfig eth0 up/down
// ifconfig eth0 |grep xxx;
int ipcm_network_exe_cmd(char *cmd, char *recvbuf, int recvlen)
{
    k_s32 retVal = -1;
    k_ipcmsg_message_t* pReq = NULL;
    k_ipcmsg_message_t* pResp = NULL;

    ipcm_socket_server_init();
    //printf("f=%s l=%d %s\n", __func__,__LINE__,cmd );    
    pReq = kd_ipcmsg_create_message(0, FUNC_NETWORK_CMD, cmd, strlen(cmd)+1);
    assert(pReq != NULL);
    if(recvlen > 0){
        recvbuf[0]=0;
        pReq->as32PrivData[0] = recvlen;
    }
        


    retVal = kd_ipcmsg_send_sync(g_ipcm_socket_clinet_s32Id, pReq, &pResp, 0x7fffffff);
    kd_ipcmsg_destroy_message(pReq);    
    
    if(retVal == 0){
        //printf("ipcm_socket ret =%x %x %x\n", pResp->s32RetVal,pResp->u32CMD, pResp->u32BodyLen);
        retVal = pResp->s32RetVal;        
        if(recvlen > pResp->u32BodyLen)
            memcpy(recvbuf, pResp->pBody, pResp->u32BodyLen);
        kd_ipcmsg_destroy_message(pResp);
    }else {
        //printf("f=%s l=%d %x \n", __func__,__LINE__, retVal);
        retVal = -1;
    }
    return retVal;
}

mp_obj_t ipcm_network_lan_ifconfig(char *netname,  size_t n_args, const mp_obj_t *args)
{
    int ret = 0;
    char cmd[128];
    char buf[800]={0};

    
    if(n_args == 0){//get ip ,mask,gw,dns
        mp_obj_t list_array[4]={mp_const_none,mp_const_none,mp_const_none,mp_const_none};

        snprintf(cmd,sizeof(cmd), "ifconfig %s | grep Mask  | cut -d: -f2 | cut -d' ' -f1", netname);
        ret += ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        if(strlen(buf) > 0)
            list_array[0] = mp_obj_new_str(buf, strlen(buf));

        //netmask
        snprintf(cmd,sizeof(cmd), "ifconfig %s | grep Mask  | cut -d: -f4 ", netname);
        ret += ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        if(strlen(buf) > 0)
            list_array[1] = mp_obj_new_str(buf, strlen(buf));
        //gw
        snprintf(cmd,sizeof(cmd), "ip route | grep  %s | grep default | cut -d' ' -f3 ", netname);
        ret += ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        if(strlen(buf) > 0)
            list_array[2] = mp_obj_new_str(buf, strlen(buf));
        //dns
        snprintf(cmd,sizeof(cmd), "cat /etc/resolv.conf  | grep nameserver | awk '{print $2}'");
        ret += ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        if(strlen(buf) > 0)
            list_array[3] = mp_obj_new_str(buf, strlen(buf));

        //printf("f=%s buf=%s  %x\n", __func__, buf,ret);
        if(ret)
            return mp_obj_new_bool(0);
        return mp_obj_new_tuple(4, list_array);

    }else if (args[0] == MP_OBJ_NEW_QSTR(MP_QSTR_dhcp)){
        //dhcp  -i eth0;
        snprintf(cmd,sizeof(cmd), "udhcpc -i %s -q >/dev/null", netname);
        ret = ipcm_network_exe_cmd(cmd, buf, sizeof(buf));
        return mp_obj_new_bool(ret == 0);
    }else  {
        //set ip mask,gw,dns;
        //ifconfig eth0 xx,xx,xx,xx,xx;
        //route add default gw 192.168.1.1 dev eth0
        //killall udhcpc
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[0], 4, &items);
        //printf("%s,%s,%s,%s\n",mp_obj_str_get_str(items[0]),mp_obj_str_get_str(items[1]),mp_obj_str_get_str(items[2]),mp_obj_str_get_str(items[3]));

        snprintf(cmd,sizeof(cmd), "killall udhcpc >/dev/null; sleep 1; ifconfig %s %s netmask %s; ", netname,  mp_obj_str_get_str(items[0]), mp_obj_str_get_str(items[1]));
        ret += ipcm_network_exe_cmd(cmd, NULL, 0);

        if(strlen(mp_obj_str_get_str(items[2])) > 0){
            snprintf(cmd,sizeof(cmd), "route del default gw %s  %s", mp_obj_str_get_str(items[2]), netname);
            ipcm_network_exe_cmd(cmd, NULL, 0);
            snprintf(cmd,sizeof(cmd), "route add default gw %s  %s", mp_obj_str_get_str(items[2]), netname);
            ret += ipcm_network_exe_cmd(cmd, NULL, 0);
        }

        
        if(strlen(mp_obj_str_get_str(items[3])) > 0){
            snprintf(cmd,sizeof(cmd), "echo \"nameserver %s # %s \">/etc/resolv.conf", mp_obj_str_get_str(items[3]), netname);
            ret += ipcm_network_exe_cmd(cmd, NULL, 0);
        }
        return mp_obj_new_bool(ret == 0);
    }
    return mp_obj_new_bool(0);
    //return mp_const_none;
}