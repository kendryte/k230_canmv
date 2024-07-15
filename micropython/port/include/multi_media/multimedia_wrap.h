#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct KdRtspServer KdRtspServer;
typedef struct SessionAttr SessionAttr;

#ifdef __cplusplus
extern "C" {
#endif

KdRtspServer * RtspServer_create();
void RtspServer_destroy(KdRtspServer *p);
int RtspServer_Init(KdRtspServer *p, int port);
void RtspServer_DeInit(KdRtspServer *p);
int RtspServer_CreateSession(KdRtspServer *p, const char *session_name, const SessionAttr *session_attr);
int RtspServer_DestroySession(KdRtspServer *p, const char *session_name);
char* RtspServer_GetRtspUrl(KdRtspServer *p, const char *session_name);
void RtspServer_Start(KdRtspServer *p);
void RtspServer_Stop(KdRtspServer *p);
int RtspServer_SendVideoData(KdRtspServer *p, const char *session_name, const uint8_t *data, size_t size, uint64_t timestamp);
int RtspServer_SendAudioData(KdRtspServer *p, const char *session_name, const uint8_t *data, size_t size, uint64_t timestamp);
char* RtspServer_Test(KdRtspServer *p);

#ifdef __cplusplus
}
#endif