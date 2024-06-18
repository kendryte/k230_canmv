#include <micropython-wrap/module.h>

extern void doinit_audio_module(mp_obj_dict_t *);

UPYWRAP_DEFINE_INIT_MODULE(audio, doinit_audio_module);
MP_REGISTER_MODULE(MP_QSTR_audio, audio_module);
MP_REGISTER_ROOT_POINTER(const mp_map_elem_t* audio_module_globals_table);
