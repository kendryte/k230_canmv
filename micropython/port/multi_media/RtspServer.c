#include "multimedia_wrap.h"
#include "multimedia_type.h"
#include "RtspServer.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "string.h"
#include <stdio.h>


// init
STATIC mp_obj_t mp_rtspserver_create() {
    rtspserver_obj_t *self = m_new_obj_with_finaliser(rtspserver_obj_t);
    self->interp = RtspServer_create();
    self->base.type = &rtsp_server_type;
    return MP_OBJ_FROM_PTR(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(rtspserver_create_obj, mp_rtspserver_create);

STATIC mp_obj_t mp_rtspserver_destroy(mp_obj_t self_in) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    RtspServer_destroy(self->interp);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rtspserver_destroy_obj, mp_rtspserver_destroy);


STATIC mp_obj_t mp_rtspserver_init(mp_obj_t self_in, mp_obj_t port) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(RtspServer_Init(self->interp, mp_obj_get_int(port)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtspserver_init_obj, mp_rtspserver_init);

STATIC mp_obj_t mp_rtspserver_deinit(mp_obj_t self_in) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    RtspServer_DeInit(self->interp);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rtspserver_deinit_obj, mp_rtspserver_deinit);

STATIC mp_obj_t mp_rtspserver_createsession(size_t n_args,const mp_obj_t *args)
{
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t session_name = args[1];
    mp_obj_t video_type = args[2];
    mp_obj_t enable_audio = args[3];
    SessionAttr attr = {true, mp_obj_get_int(enable_audio), false, mp_obj_get_int(video_type)};
    const SessionAttr *session_attr_obj = &attr;
    return mp_obj_new_int(RtspServer_CreateSession(self->interp, mp_obj_str_get_str(session_name), session_attr_obj));

}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtspserver_createsession_obj, 4,4,mp_rtspserver_createsession);

STATIC mp_obj_t mp_rtspserver_destroysession(mp_obj_t self_in, mp_obj_t session_name) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(RtspServer_DestroySession(self->interp, mp_obj_str_get_str(session_name)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtspserver_destroysession_obj, mp_rtspserver_destroysession);

STATIC mp_obj_t mp_rtspserver_getrtspurl(mp_obj_t self_in, mp_obj_t session_name) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char* url = RtspServer_GetRtspUrl(self->interp, mp_obj_str_get_str(session_name));
    mp_obj_t mp_url_string;
    mp_url_string = mp_obj_new_str(url, strlen(url));
    return MP_OBJ_TO_PTR(mp_url_string);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(rtspserver_getrtspurl_obj, mp_rtspserver_getrtspurl);

STATIC mp_obj_t mp_rtspserver_start(mp_obj_t self_in) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    RtspServer_Start(self->interp);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rtspserver_start_obj, mp_rtspserver_start);

STATIC mp_obj_t mp_rtspserver_stop(mp_obj_t self_in) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    RtspServer_Stop(self->interp);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(rtspserver_stop_obj, mp_rtspserver_stop);

STATIC mp_obj_t mp_rtspserver_sendvideodata(size_t n_args,const mp_obj_t *args) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t session_name = args[1];
    mp_obj_t data = args[2];
    mp_obj_t size = args[3];
    mp_obj_t timestamp = args[4];

    return mp_obj_new_int(RtspServer_SendVideoData(self->interp, mp_obj_str_get_str(session_name), (const uint8_t *)mp_obj_str_get_str(data), mp_obj_get_int(size), mp_obj_get_int(timestamp)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtspserver_sendvideodata_obj, 5,5,mp_rtspserver_sendvideodata);

STATIC mp_obj_t mp_rtspserver_sendaudiodata(size_t n_args,const mp_obj_t *args) {
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t session_name = args[1];
    mp_obj_t data = args[2];
    mp_obj_t size = args[3];
    mp_obj_t timestamp = args[4];
    return mp_obj_new_int(RtspServer_SendAudioData(self->interp, mp_obj_str_get_str(session_name), (const uint8_t *)mp_obj_str_get_str(data), mp_obj_get_int(size), mp_obj_get_int(timestamp)));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtspserver_sendaudiodata_obj,5,5, mp_rtspserver_sendaudiodata);

STATIC mp_obj_t mp_rtspserver_test(mp_obj_t self_in)
{
    rtspserver_obj_t *self = MP_OBJ_TO_PTR(self_in);
    char* v = RtspServer_Test(self->interp);
    mp_obj_t mp_v_string;
    mp_v_string = mp_obj_new_str(v, strlen(v));
    return MP_OBJ_TO_PTR(mp_v_string);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_rtsp_test_obj, mp_rtspserver_test);

STATIC const mp_rom_map_elem_t RtspServer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_rtspserver) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_create), MP_ROM_PTR(&rtspserver_create_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_destroy), MP_ROM_PTR(&rtspserver_destroy_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_init), MP_ROM_PTR(&rtspserver_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_deinit), MP_ROM_PTR(&rtspserver_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_createsession), MP_ROM_PTR(&rtspserver_createsession_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_destroysession), MP_ROM_PTR(&rtspserver_destroysession_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_getrtspurl), MP_ROM_PTR(&rtspserver_getrtspurl_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_start), MP_ROM_PTR(&rtspserver_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_stop), MP_ROM_PTR(&rtspserver_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_sendvideodata), MP_ROM_PTR(&rtspserver_sendvideodata_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_sendaudiodata), MP_ROM_PTR(&rtspserver_sendaudiodata_obj) },
    { MP_ROM_QSTR(MP_QSTR_rtspserver_test), MP_ROM_PTR(&mp_rtsp_test_obj) },
};

STATIC MP_DEFINE_CONST_DICT(RtspServer_locals_dict, RtspServer_locals_dict_table);

const mp_obj_module_t mp_module_rtspserver = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&RtspServer_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE (MP_QSTR_rtspserver, mp_module_rtspserver);

MP_DEFINE_CONST_OBJ_TYPE(
    rtsp_server_type,
    MP_QSTR_rtspserver,
    MP_TYPE_FLAG_NONE,
    make_new,mp_rtspserver_create,
    locals_dict, &RtspServer_locals_dict
    );

// interp_mode
STATIC const mp_rom_map_elem_t mp_media_type_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_media_h264), MP_ROM_INT(MULTI_MEDIA_TYPE_H264) },
    { MP_ROM_QSTR(MP_QSTR_media_h265), MP_ROM_INT(MULTI_MEDIA_TYPE_H265) },
    { MP_ROM_QSTR(MP_QSTR_media_mjpeg), MP_ROM_INT(MULTI_MEDIA_TYPE_MJPEG) },
};
STATIC MP_DEFINE_CONST_DICT(mp_media_type_locals_dict, mp_media_type_locals_dict_table);

const mp_obj_module_t mp_module_media_type = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_media_type_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_multi_media_type, mp_module_media_type);

MP_DEFINE_CONST_OBJ_TYPE(
    multi_media_type,
    MP_QSTR_multi_media_type,
    MP_TYPE_FLAG_INSTANCE_TYPE,
    locals_dict, &mp_media_type_locals_dict
    );
