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
#include "sys/ioctl.h"
#include "machine_adc.h"
#include "py/runtime.h"
#include "py/obj.h"

#define ADC_DEVIDE_NAME "/dev/adc"
#define REF_VOL         (1.8)
#define RESOLUTION      (4096 - 1)
#define ADC_CHANNEL_MAX 6

typedef struct {
    mp_obj_base_t base;
    uint8_t channel;
} machine_adc_obj_t;

static int adc_fd = -1;

static int adc_read_value(uint8_t channel) {
    uint32_t value;

    lseek(adc_fd, channel, SEEK_SET);
    read(adc_fd, &value, sizeof(value));

    return value;
}

STATIC void machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC: %d", self->channel);
}

STATIC mp_obj_t machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, true);

    if (adc_fd < 0) {
        adc_fd = open(ADC_DEVIDE_NAME, O_RDWR);
        if (adc_fd < 0) {
            mp_raise_OSError_with_filename(errno, ADC_DEVIDE_NAME);
        }
    }

    int channel = mp_obj_get_int(args[0]);
    if (channel < 0 || channel >= ADC_CHANNEL_MAX) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("channel is invalid"));
    }

    machine_adc_obj_t *self = mp_obj_malloc(machine_adc_obj_t, &machine_adc_type);
    self->channel = channel;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_adc_read_u16(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int_from_uint(adc_read_value(self->channel));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_u16_obj, machine_adc_read_u16);

STATIC mp_obj_t machine_adc_read_uv(mp_obj_t self_in) {
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int_from_uint((uint32_t)(adc_read_value(self->channel) * REF_VOL * 1e6 / RESOLUTION));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_read_uv_obj, machine_adc_read_uv);

STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read_u16), MP_ROM_PTR(&machine_adc_read_u16_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_uv), MP_ROM_PTR(&machine_adc_read_uv_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_adc_type,
    MP_QSTR_ADC,
    MP_TYPE_FLAG_NONE,
    make_new, machine_adc_make_new,
    print, machine_adc_print,
    locals_dict, &machine_adc_locals_dict
    );
