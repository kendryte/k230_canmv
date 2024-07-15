#include "multimedia_wrap.h"
#include "rtsp_server.h"

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

char* RtspServer_Test(KdRtspServer *p)
{
    return "hello world";
}
