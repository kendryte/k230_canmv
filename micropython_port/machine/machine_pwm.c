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
#include "machine_pwm.h"
#include "py/runtime.h"
#include "py/obj.h"

#define PWM_DEVICE_NAME     "/dev/pwm"
#define PWM_CHANNEL_MAX     6
#define	KD_PWM_CMD_ENABLE   _IOW('P', 0, int)
#define	KD_PWM_CMD_DISABLE  _IOW('P', 1, int)
#define	KD_PWM_CMD_SET      _IOW('P', 2, int)
#define	KD_PWM_CMD_GET      _IOW('P', 3, int)

typedef struct _machine_pwm_obj_t {
    mp_obj_base_t base;
    uint8_t channel;
    double freq;
    double duty;
    bool active;
    bool valid;
} machine_pwm_obj_t;

typedef struct {
    uint32_t channel; /* 0-5 */
    uint32_t period;  /* unit:ns 1ns~4.29s:1Ghz~0.23hz */
    uint32_t pulse;   /* unit:ns (pulse<=period) */
} pwm_config_t;

typedef struct {
    int fd;
    int refcnt;
    uint8_t used[PWM_CHANNEL_MAX];
} k230_pwm_obj_t;

STATIC k230_pwm_obj_t k230_pwm_obj = {
    .fd = -1,
};

/******************************************************************************/
// MicroPython bindings for PWM

STATIC void mp_machine_pwm_obj_check(machine_pwm_obj_t *self) {
    if (self->valid == 0)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The PWM channel object has been deleted"));
}

STATIC void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    config.channel = self->channel;
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_GET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u get config error"), self->channel);
    mp_printf(print, "pwm chanel %u: period = %u ns, pulse = %u ns, %s", self->channel,
        config.period, config.pulse, self->active ? "enabled" : "disabled");
}

STATIC void mp_machine_pwm_init_helper(machine_pwm_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq, ARG_duty, ARG_enable };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_duty, MP_ARG_OBJ, {.u_obj  = mp_const_none} },
        { MP_QSTR_enable,  MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->freq = mp_obj_get_float(args[ARG_freq].u_obj);
    self->duty = mp_const_none == args[ARG_duty].u_obj ? 50.0 : mp_obj_get_float(args[ARG_duty].u_obj);
    self->freq = self->freq <= 0 ? 1 : self->freq;
    self->duty = self->duty < 0 ? 0 : self->duty > 100 ? 100 : self->duty;

    pwm_config_t config;
    config.channel = self->channel;
    config.period = (uint32_t)(1000000000.0 / self->freq);
    config.pulse = (uint32_t)(self->duty / 100 * config.period);
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_SET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u config error"), self->channel);
    self->active = args[ARG_enable].u_bool;
    if (self->active && ioctl(k230_pwm_obj.fd, KD_PWM_CMD_ENABLE, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u enable error"), self->channel);
}

STATIC mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    int channel;

    mp_arg_check_num(n_args, n_kw, 2, 4, true);

    if (mp_obj_is_int(args[0])) {
        channel = mp_obj_get_int(args[0]);
        if (channel < 0 || channel >= PWM_CHANNEL_MAX)
            mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
        if (k230_pwm_obj.used[channel])
            mp_raise_ValueError(MP_ERROR_TEXT("channel busy"));
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
    }

    if (k230_pwm_obj.refcnt == 0 && k230_pwm_obj.fd == -1) {
        k230_pwm_obj.fd = open(PWM_DEVICE_NAME, O_RDWR);
        if (k230_pwm_obj.fd < 0)
            mp_raise_OSError_with_filename(errno, PWM_DEVICE_NAME);
    }

    machine_pwm_obj_t *self = m_new_obj_with_finaliser(machine_pwm_obj_t);
    self->base.type = &machine_pwm_type;
    self->channel = channel;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_pwm_init_helper(self, n_args - 1, args + 1, &kw_args);

    k230_pwm_obj.used[channel] = 1;
    k230_pwm_obj.refcnt++;
    self->valid = true;

    return MP_OBJ_FROM_PTR(self);
}

STATIC void mp_machine_pwm_deinit(machine_pwm_obj_t *self) {
    if (self->valid == 0)
        return;

    self->valid = 0;
    k230_pwm_obj.used[self->channel] = 0;

    if (k230_pwm_obj.refcnt)
        k230_pwm_obj.refcnt--;

    if (k230_pwm_obj.refcnt == 0) {
        close(k230_pwm_obj.fd);
        k230_pwm_obj.fd = -1;
    }
}

STATIC mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self) {
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    config.channel = self->channel;
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_GET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u get config error"), self->channel);

    return mp_obj_new_float_from_d(1000000000.0 / config.period);
}

STATIC void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_float_t freq) {
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    self->freq = freq <= 0 ? 1 : freq;
    config.channel = self->channel;
    config.period = (uint32_t)(1000000000.0 / self->freq);
    config.pulse = (uint32_t)(self->duty / 100 * config.period);
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_SET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u config error"), self->channel);
}

STATIC mp_obj_t mp_machine_pwm_duty_get(machine_pwm_obj_t *self) {
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    config.channel = self->channel;
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_GET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u get config error"), self->channel);

    return mp_obj_new_float_from_d((double)config.pulse * 100 / (double)config.period);
}

STATIC void mp_machine_pwm_duty_set(machine_pwm_obj_t *self, mp_float_t duty) {
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    self->duty = duty < 0 ? 0 : duty > 100 ? 100 : duty;
    config.channel = self->channel;
    config.period = (uint32_t)(1000000000.0 / self->freq);
    config.pulse = (uint32_t)(self->duty / 100 * config.period);
    if (ioctl(k230_pwm_obj.fd, KD_PWM_CMD_SET, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u config error"), self->channel);
}

STATIC void mp_machine_pwm_enable(machine_pwm_obj_t *self, bool enable) {
    pwm_config_t config;

    mp_machine_pwm_obj_check(self);
    config.channel = self->channel;
    self->active = enable;
    if (ioctl(k230_pwm_obj.fd, self->active ? KD_PWM_CMD_ENABLE : KD_PWM_CMD_DISABLE, &config))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("PWM channel %u %s error"),
            self->channel, self->active ? "enabled" : "disabled");
}

STATIC mp_obj_t machine_pwm_del(mp_obj_t self_in) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_machine_pwm_deinit(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_del_obj, machine_pwm_del);

STATIC mp_obj_t machine_pwm_deinit(mp_obj_t self_in) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_machine_pwm_obj_check(self);
    mp_machine_pwm_deinit(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pwm_deinit_obj, machine_pwm_deinit);

STATIC mp_obj_t machine_pwm_freq(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        // Get frequency.
        return mp_machine_pwm_freq_get(self);
    } else {
        // Set the frequency.
        mp_float_t freq = mp_obj_get_float(args[1]);
        mp_machine_pwm_freq_set(self, freq);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_freq_obj, 1, 2, machine_pwm_freq);

STATIC mp_obj_t machine_pwm_duty(size_t n_args, const mp_obj_t *args) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        // Get duty cycle.
        return mp_machine_pwm_duty_get(self);
    } else {
        // Set duty cycle.
        mp_float_t duty = mp_obj_get_float(args[1]);
        mp_machine_pwm_duty_set(self, duty);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pwm_duty_obj, 1, 2, machine_pwm_duty);

STATIC mp_obj_t machine_pwm_enable(mp_obj_t self_o, mp_obj_t enable) {
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_o);

    mp_machine_pwm_enable(self, mp_obj_is_true(enable));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_pwm_enable_obj, machine_pwm_enable);

STATIC const mp_rom_map_elem_t machine_pwm_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_pwm_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_pwm_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_pwm_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty), MP_ROM_PTR(&machine_pwm_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_enable), MP_ROM_PTR(&machine_pwm_enable_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pwm_locals_dict, machine_pwm_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pwm_type,
    MP_QSTR_PWM,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_pwm_make_new,
    print, mp_machine_pwm_print,
    locals_dict, &machine_pwm_locals_dict
    );
