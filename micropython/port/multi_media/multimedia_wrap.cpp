#include "multimedia_wrap.h"
#include "rtsp_server.h"
#include "mpi_sys_api.h"

KdRtspServer * RtspServer_create() {
    return new KdRtspServer();
}

void RtspServer_destroy(KdRtspServer *p) {
    delete p;
    p = nullptr;
}

int RtspServer_Init(KdRtspServer *p, int port) {
    return p->Init(port, nullptr);
}

void RtspServer_DeInit(KdRtspServer *p) {
    p->DeInit();
}

int RtspServer_CreateSession(KdRtspServer *p, const char *session_name, const SessionAttr *session_attr) {
    return p->CreateSession(session_name, *session_attr);
}

int RtspServer_DestroySession(KdRtspServer *p, const char *session_name) {
    printf("RtspServer_DestroySession  before session_name:%s\n",session_name);
    int ret = p->DestroySession(session_name);
    printf("RtspServer_DestroySession  after session_name:%s\n",session_name);
    return ret;
}

char* RtspServer_GetRtspUrl(KdRtspServer *p, const char *session_name) {
    return p->GetRtspUrl(session_name);
}

void RtspServer_Start(KdRtspServer *p) {
    p->Start();
}

void RtspServer_Stop(KdRtspServer *p) {
    p->Stop();
}

int RtspServer_SendVideoData(KdRtspServer *p, const char *session_name, const uint8_t *data, size_t size, uint64_t timestamp) {
    //printf("RtspServer_SendVideoData session_name:%s,size:%ld,timestamp:%ld\n",session_name,size,timestamp);
    return p->SendVideoData(session_name, data, size, timestamp);
}

int RtspServer_SendAudioData(KdRtspServer *p, const char *session_name, const uint8_t *data, size_t size, uint64_t timestamp) {
    return p->SendAudioData(session_name, data, size, timestamp);
}


int RtspServer_SendVideoData_Byphyaddr(KdRtspServer *p, const char *session_name, uint64_t phy_addr, size_t size, uint64_t timestamp){
    uint8_t * vir_data = (uint8_t *)kd_mpi_sys_mmap(phy_addr, size);
    p->SendVideoData(session_name, vir_data, size, timestamp);

    kd_mpi_sys_munmap((void*)vir_data, size);
    //printf("RtspServer_SendVideoData_Byphyaddr session_name:%s,phy_addr:%ld,size:%ld,timestamp:%ld\n",session_name,phy_addr,size,timestamp);
    return 0;
}


int RtspServer_SendAudioData_Byphyaddr(KdRtspServer *p, const char *session_name, uint64_t phy_addr, size_t size, uint64_t timestamp){
    uint8_t * vir_data = (uint8_t *)kd_mpi_sys_mmap(phy_addr, size);
    p->SendAudioData(session_name, vir_data, size, timestamp);

    kd_mpi_sys_munmap((void*)vir_data, size);
    return 0;
}

char* RtspServer_Test(KdRtspServer *p)
{
    return "hello world";
}
