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
#include "machine_uart.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/stream.h"

#define IOC_SET_BAUDRATE    _IOW('U', 0x40, int)

typedef struct {
    mp_obj_base_t base;
    int fd;
    uint32_t baudrate;
    uint8_t index;
    uint8_t bitwidth;
    uint8_t parity;
    uint8_t stop;
    uint8_t status;
} machine_uart_obj_t;

struct uart_configure {
    uint32_t baud_rate;
    uint32_t data_bits  : 4;
    uint32_t stop_bits  : 2;
    uint32_t parity     : 2;
    uint32_t fifo_lenth : 2;
    uint32_t auto_flow  : 1;
    uint32_t reserved   : 21;
};

static bool uart_used[5] = {1, 0, 0, 1, 0};

STATIC void machine_uart_obj_check(machine_uart_obj_t *self) {
    if (self->status == 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The UART object has been deleted"));
    }
}

STATIC void machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_uart_obj_check(self);
    mp_printf(print, "UART %u: baudrate=%u, bitwidth=%u, parity=%u, stop=%u", self->index,
        self->baudrate, self->bitwidth, self->parity, self->stop);
}

STATIC void machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->baudrate = args[ARG_baudrate].u_int;
    self->bitwidth = args[ARG_bits].u_int;
    self->parity = args[ARG_parity].u_int;
    self->stop = args[ARG_stop].u_int;

    struct uart_configure conf;
    conf.baud_rate = self->baudrate;
    conf.data_bits = self->bitwidth;
    conf.parity = self->parity;
    conf.stop_bits = self->stop;
    conf.fifo_lenth = 2;
    conf.auto_flow = 0;

    ioctl(self->fd, IOC_SET_BAUDRATE, &conf);
}

STATIC mp_obj_t machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 5, true);

    int index = mp_obj_get_int(args[0]);
    if (index < 0 || index > 4) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid uart number"));
    }
    if (uart_used[index]) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("UART %u busy"), index);
    }

    char dev_name[16] = "/dev/uart0";
    dev_name[9] = '0' + index;
    int fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        mp_raise_OSError_with_filename(errno, dev_name);
    }

    machine_uart_obj_t *self = m_new_obj_with_finaliser(machine_uart_obj_t);
    self->base.type = &machine_uart_type;
    self->index = index;
    self->fd = fd;
    self->status = 1;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    self->status = 2;
    uart_used[index] = 1;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_uart_deinit(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_uart_obj_check(self);
    close(self->fd);
    if (self->status == 2)
        uart_used[self->index] = 0;
    self->status = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit);

STATIC mp_obj_t machine_uart_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_uart_obj_check(args[0]);
    machine_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 1, machine_uart_init);

STATIC mp_uint_t machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_uart_obj_check(self);

    size = read(self->fd, buf_in, size);
    *errcode = 0;

    return size;
}

STATIC mp_uint_t machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_uart_obj_check(self);

    write(self->fd, buf_in, size);
    *errcode = 0;

    return size;
}

STATIC mp_uint_t machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_uint_t ret;

    *errcode = MP_EINVAL;
    ret = MP_STREAM_ERROR;

    return ret;
}

STATIC const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },

    { MP_ROM_QSTR(MP_QSTR_UART1), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_UART2), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_UART3), MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_UART4), MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_FIVEBITS), MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_SIXBITS), MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_SEVENBITS), MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_EIGHTBITS), MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_STOPBITS_ONE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_STOPBITS_TWO), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_NONE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ODD), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_EVEN), MP_ROM_INT(3) },
};
STATIC MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

STATIC const mp_stream_p_t machine_uart_stream_p = {
    .read = machine_uart_read,
    .write = machine_uart_write,
    .ioctl = machine_uart_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_uart_type,
    MP_QSTR_UART,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    make_new, machine_uart_make_new,
    print, machine_uart_print,
    protocol, &machine_uart_stream_p,
    locals_dict, &machine_uart_locals_dict
    );
