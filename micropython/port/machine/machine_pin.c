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
#include <signal.h>
#include <errno.h>
#include "sys/ioctl.h"
#include "machine_pin.h"
#include "machine_fpioa.h"
#include "py/runtime.h"
#include "py/obj.h"

#define PULL_NONE 0
#define PULL_DOWN 1
#define PULL_UP 2

#define DIR_OUT 0
#define DIR_IN 1

/* ioctl */
#define GPIO_DEVICE_NAME "/dev/gpio"
#define MAX_GPIO_NUM 63
#define GPIO_DM_OUTPUT _IOW('G', 0, int)
#define GPIO_DM_INPUT _IOW('G', 1, int)
#define GPIO_DM_INPUT_PULL_UP _IOW('G', 2, int)
#define GPIO_DM_INPUT_PULL_DOWN _IOW('G', 3, int)
#define GPIO_WRITE_LOW _IOW('G', 4, int)
#define GPIO_WRITE_HIGH _IOW('G', 5, int)
#define KD_GPIO_PE_RISING _IOW('G', 7, int)
#define KD_GPIO_PE_FALLING _IOW('G', 8, int)
#define KD_GPIO_PE_BOTH _IOW('G', 9, int)
#define KD_GPIO_PE_HIGH _IOW('G', 10, int)
#define KD_GPIO_PE_LOW _IOW('G', 11, int)
#define GPIO_READ_VALUE _IOW('G', 12, int)
#define KD_GPIO_ENABLE_IRQ _IOW('G', 13, int)
#define KD_GPIO_DISABLE_IRQ _IOW('G', 14, int)
#define KD_GPIO_ATTACH_IRQ _IOW('G', 15, int)
#define KD_GPIO_DETACH_IRQ _IOW('G', 16, int)

typedef struct {
    uint16_t pin;
    uint16_t value;
} gpio_config_t;

typedef struct {
    mp_obj_base_t base;
    uint8_t pin;
    uint8_t mode;
} machine_pin_obj_t;

static int gpio_fd = -1;

STATIC void machine_pin_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value, ARG_drive };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_pull, MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_drive, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    int pin = self->pin;
    int mode = args[ARG_mode].u_int == DIR_OUT ? DIR_OUT : DIR_IN;
    int pull = args[ARG_pull].u_int;
    int value = args[ARG_value].u_int;
    int ds = args[ARG_drive].u_int;
    gpio_config_t gpio_cfg = { .pin = pin };
    struct st_iomux_reg reg_cfg;

    if (pull != -1 || ds != -1) {
        fpioa_drv_reg_get(pin, &reg_cfg.u.value);
        if (pull != -1) {
            pull = pull < 0 ? 0 : pull;
            reg_cfg.u.bit.pu = (pull & PULL_UP) ? 1 : 0;
            reg_cfg.u.bit.pd = (pull & PULL_DOWN) ? 1 : 0;
        }
        if (ds != -1) {
            reg_cfg.u.bit.ds = ds < 0 ? 0 : ds > 0xF ? 0xF : ds;
        }
        fpioa_drv_reg_set(pin, reg_cfg.u.value);
    }

    if (mode == DIR_OUT) {
        if (ioctl(gpio_fd, GPIO_DM_OUTPUT, &gpio_cfg)) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio set output error"));
        }
        if (value != -1) {
            if (ioctl(gpio_fd, value > 0 ? GPIO_WRITE_HIGH : GPIO_WRITE_LOW, &gpio_cfg)) {
                mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio set output level error"));
            }
        }
    } else {
        if (ioctl(gpio_fd, GPIO_DM_INPUT, &gpio_cfg)) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio set input error"));
        }
    }
    self->mode = mode;
}

STATIC void machine_pin_value_set(machine_pin_obj_t *self, int value) {
    gpio_config_t gpio_cfg = { .pin = self->pin };

    if (ioctl(gpio_fd, value > 0 ? GPIO_WRITE_HIGH : GPIO_WRITE_LOW, &gpio_cfg)) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio set output level error"));
    }
}

STATIC int machine_pin_value_get(machine_pin_obj_t *self) {
    gpio_config_t gpio_cfg = { .pin = self->pin };

    if (ioctl(gpio_fd, GPIO_READ_VALUE, &gpio_cfg)) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio read input level error"));
    }

    return gpio_cfg.value;
}

STATIC void machine_pin_mode_set(machine_pin_obj_t *self, int value) {
    gpio_config_t gpio_cfg = { .pin = self->pin };

    if (ioctl(gpio_fd, value == DIR_OUT ? GPIO_DM_OUTPUT : GPIO_DM_INPUT, &gpio_cfg)) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("gpio set dir error"));
    }

    self->mode = value == DIR_OUT ? DIR_OUT : DIR_IN;
}

STATIC int machine_pin_mode_get(machine_pin_obj_t *self) {
    return self->mode;
}

STATIC void machine_pin_pull_set(machine_pin_obj_t *self, int value) {
    struct st_iomux_reg reg_cfg;

    fpioa_drv_reg_get(self->pin, &reg_cfg.u.value);
    value = value < 0 ? 0 : value;
    reg_cfg.u.bit.pu = (value & PULL_UP) ? 1 : 0;
    reg_cfg.u.bit.pd = (value & PULL_DOWN) ? 1 : 0;
    fpioa_drv_reg_set(self->pin, reg_cfg.u.value);
}

STATIC int machine_pin_pull_get(machine_pin_obj_t *self) {
    struct st_iomux_reg reg_cfg;
    int value = 0;

    fpioa_drv_reg_get(self->pin, &reg_cfg.u.value);
    value |= (reg_cfg.u.bit.pu ? PULL_UP : 0);
    value |= (reg_cfg.u.bit.pd ? PULL_DOWN : 0);

    return value;
}

STATIC void machine_pin_drive_set(machine_pin_obj_t *self, int value) {
    struct st_iomux_reg reg_cfg;

    fpioa_drv_reg_get(self->pin, &reg_cfg.u.value);
    reg_cfg.u.bit.ds = value < 0 ? 0 : value > 0xF ? 0xF : value;
    fpioa_drv_reg_set(self->pin, reg_cfg.u.value);
}

STATIC int machine_pin_drive_get(machine_pin_obj_t *self) {
    struct st_iomux_reg reg_cfg;

    fpioa_drv_reg_get(self->pin, &reg_cfg.u.value);

    return reg_cfg.u.bit.ds;
}

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "Pin: %d, mode: %d, value: %d, pull: %d, drive: %d",
        self->pin, machine_pin_mode_get(self), machine_pin_value_get(self),
        machine_pin_pull_get(self), machine_pin_drive_get(self));
}

STATIC mp_obj_t machine_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 5, true);

    if (gpio_fd < 0) {
        gpio_fd = open(GPIO_DEVICE_NAME, O_RDWR);
        if (gpio_fd < 0) {
            mp_raise_OSError_with_filename(errno, GPIO_DEVICE_NAME);
        }
    }

    int pin = mp_obj_get_int(args[0]);
    if (pin < 0 || pin > MAX_GPIO_NUM) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin number is invalid"));
    }

    machine_pin_obj_t *self = mp_obj_malloc(machine_pin_obj_t, &machine_pin_type);
    self->pin = pin;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_pin_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_pin_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_pin_init_helper(args[0], n_args - 1, args + 1, kw_args);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 2, machine_pin_init);

STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(machine_pin_value_get(self));
    } else {
        machine_pin_value_set(self, mp_obj_get_int(args[1]));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    machine_pin_value_set(self, 1);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    machine_pin_value_set(self, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

STATIC mp_obj_t machine_pin_mode(size_t n_args, const mp_obj_t *args) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(machine_pin_mode_get(self));
    } else {
        machine_pin_mode_set(self, mp_obj_get_int(args[1]));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_mode_obj, 1, 2, machine_pin_mode);

STATIC mp_obj_t machine_pin_pull(size_t n_args, const mp_obj_t *args) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(machine_pin_pull_get(self));
    } else {
        machine_pin_pull_set(self, mp_obj_get_int(args[1]));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_pull_obj, 1, 2, machine_pin_pull);

STATIC mp_obj_t machine_pin_drive(size_t n_args, const mp_obj_t *args) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(machine_pin_drive_get(self));
    } else {
        machine_pin_drive_set(self, mp_obj_get_int(args[1]));
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_drive_obj, 1, 2, machine_pin_drive);

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_low), MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_high), MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&machine_pin_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_pull), MP_ROM_PTR(&machine_pin_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_drive), MP_ROM_PTR(&machine_pin_drive_obj) },
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(DIR_IN) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(DIR_OUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE), MP_ROM_INT(PULL_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(PULL_DOWN) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, machine_pin_make_new,
    print, machine_pin_print,
    locals_dict, &machine_pin_locals_dict
    );
