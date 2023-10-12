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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "sys/ioctl.h"
#include "machine_adc.h"
#include "py/runtime.h"
#include "py/obj.h"

#define ADC_CHN_ENABLE  (0)
#define ADC_CHN_DISABLE (1)
#define REF_VOL         (1.8)
#define RESOLUTION      (4096)

#define DEVIDE_ADC "/dev/adc"

typedef struct _machine_adc_obj_t {
    mp_obj_base_t base;
    int fd;
    uint32_t channel;
} machine_adc_obj_t;


STATIC void mp_machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = self_in;
    mp_printf(print, 
        "[CANMV]ADC:(%p; channel=%d)",self, self->channel);
}

STATIC mp_obj_t machine_adc_obj_init_helper(machine_adc_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    bool enable;
    enum { ARG_channel, ARG_enable };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_channel, MP_ARG_INT},
        { MP_QSTR_enable, MP_ARG_BOOL | MP_ARG_KW_ONLY,{.u_bool = true} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->channel = args[ARG_channel].u_int;
    enable = args[ARG_enable].u_bool;
    uint32_t *p = (uint32_t *)(intptr_t)(self->channel);
    if(enable == true)
    {
        if(ioctl(self->fd,ADC_CHN_ENABLE,(void *)p))
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("machine_adc_init error"));
    }

    return mp_const_none;
}

STATIC mp_obj_t mp_machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    
    mp_arg_check_num(n_args, n_kw, 0, 1, true);
    machine_adc_obj_t *self = mp_obj_malloc(machine_adc_obj_t, &machine_adc_type);
    self->fd = open(DEVIDE_ADC, O_RDWR);
    if(self->fd < 0)
        mp_raise_OSError_with_filename(errno, DEVIDE_ADC);
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_adc_obj_init_helper(self, n_args, args, &kw_args);


    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_adc_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    // machine_adc_obj_t *self = m_new_obj(machine_adc_obj_t);
    // self->base.type = &machine_adc_type;
	return machine_adc_obj_init_helper(args[0], n_args - 1, args + 1,kw_args);

    // return MP_OBJ_FROM_PTR(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_adc_init_obj, 1, machine_adc_init);

STATIC mp_obj_t machine_adc_value(mp_obj_t self_in) 
{
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    unsigned int reg_value = 0;
    // float vol = 0.0;
    rt_device_t adc_dev;
    adc_dev = rt_device_find("adc");
    rt_device_read(adc_dev,self->channel,(void *)&reg_value, sizeof(uint32_t));

    // vol = REF_VOL * reg_value / RESOLUTION;
    // printf("channel %d reg_value:0x%04x, voltage:%f\n", self->channel, reg_value, vol);

    return mp_obj_new_int_from_uint(reg_value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_value_obj,machine_adc_value);

STATIC mp_obj_t machine_adc_deinit(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t *p = (uint32_t *)(intptr_t)(self->channel);
    if(ioctl(self->fd,ADC_CHN_DISABLE,(void *)p))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("machine_adc_deinit error"));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_deinit_obj, machine_adc_deinit);

STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_adc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_adc_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_adc_value_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_adc_type,
    MP_QSTR_ADC,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_adc_make_new,
    print, mp_machine_adc_print,
    locals_dict, &machine_adc_locals_dict
    );