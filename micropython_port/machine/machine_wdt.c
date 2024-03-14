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
#include "machine_wdt.h"
#include "py/runtime.h"
#include "py/obj.h"

#define CTRL_WDT_GET_TIMEOUT    _IOW('W', 1, int) /* get timeout(in seconds) */
#define CTRL_WDT_SET_TIMEOUT    _IOW('W', 2, int) /* set timeout(in seconds) */
#define CTRL_WDT_GET_TIMELEFT   _IOW('W', 3, int) /* get the left time before reboot(in seconds) */
#define CTRL_WDT_KEEPALIVE      _IOW('W', 4, int) /* refresh watchdog */
#define CTRL_WDT_START          _IOW('W', 5, int) /* start watchdog */
#define CTRL_WDT_STOP           _IOW('W', 6, int) /* stop watchdog */

typedef enum _wdt_device_number {
    WDT_DEVICE_0,
    WDT_DEVICE_1,
    WDT_DEVICE_MAX,
} wdt_device_number_t;

typedef struct _machine_wdt_obj_t {
    mp_obj_base_t base;
    wdt_device_number_t id;
    uint32_t timeout;
    int wdt_fd;
} machine_wdt_obj_t;

STATIC uint8_t used[WDT_DEVICE_MAX];

STATIC void mp_machine_wdt_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_wdt_obj_t *self = self_in;
    mp_printf(print, "WDT %u: timeout=%ds", self->id, self->timeout);
}

STATIC mp_obj_t mp_machine_wdt_make_new(const mp_obj_type_t *type_in, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_timeout};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_timeout, MP_ARG_INT, {.u_int = 5} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_id].u_int < WDT_DEVICE_0 || args[ARG_id].u_int >= WDT_DEVICE_MAX)
        mp_raise_ValueError("WDT id does not exist");

    if (args[ARG_timeout].u_int <= 0)
        mp_raise_ValueError("WDT timeout too short");

    if (used[args[ARG_id].u_int] == 1)
        mp_raise_ValueError("WDT id is used");

    machine_wdt_obj_t *self = mp_obj_malloc(machine_wdt_obj_t, &machine_wdt_type);
    self->id = args[ARG_id].u_int;
    self->timeout  = args[ARG_timeout].u_int;

    char device_name[16];
    sprintf(device_name,"/dev/watchdog%u", self->id);
    int fd = open(device_name, O_RDWR);
    if (fd < 0)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("open wdt error"));

    if (ioctl(fd, CTRL_WDT_SET_TIMEOUT, &self->timeout))
        goto err;
    if (ioctl(fd, CTRL_WDT_START, NULL))
        goto err;

    self->wdt_fd = fd;
    used[self->id] = 1;
    return self;
err:
    close(fd);
    mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("wdt init error"));  
    return mp_const_none;
}

STATIC mp_obj_t machine_wdt_feed(mp_obj_t self_in) {
    machine_wdt_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (ioctl(self->wdt_fd, CTRL_WDT_KEEPALIVE, NULL))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("feed watchdog error"));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_wdt_feed_obj, machine_wdt_feed);

STATIC const mp_rom_map_elem_t machine_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&machine_wdt_feed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(machine_wdt_locals_dict, machine_wdt_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_wdt_type,
    MP_QSTR_WDT,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_wdt_make_new,
    print, mp_machine_wdt_print,
    locals_dict, &machine_wdt_locals_dict
    );
