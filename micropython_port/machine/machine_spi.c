/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "py/runtime.h"
#include "machine_spi.h"
#include "sys/ioctl.h"
#include "py/obj.h"

#define SPI_DEVICE_NAME "/dev/spi"
#define SPI0_DEVICE_NAME "/dev/spi0"
#define SPI1_DEVICE_NAME "/dev/spi1"
#define SPI2_DEVICE_NAME "/dev/spi2"

#define RT_SPI_DEV_CTRL_CONFIG (0xc00 + 0x01)
#define RT_SPI_DEV_CTRL_RW (0xc00 + 0x02)
#define RT_SPI_DEV_CTRL_CLK (0xc00 + 0x03)

struct rt_spi_priv_data {
    const void *send_buf;
    size_t send_length;
    void *recv_buf;
    size_t recv_length;
};

struct rt_spi_configuration {
    uint8_t mode;
    uint8_t data_width;
    uint16_t reserved;
    uint32_t max_hz;
};

enum {
    DWENUM_SPI_TXRX = 0,
    DWENUM_SPI_TX = 1,
    DWENUM_SPI_RX = 2,
    DWENUM_SPI_EEPROM = 3
};

enum {
    MP_SPI_IOCTL_INIT,
    MP_SPI_IOCTL_DEINIT,
};

typedef struct {
    uint32_t baud;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t reserver;
} mp_soft_spi_obj_t;

typedef struct {
    mp_obj_base_t base;
    mp_soft_spi_obj_t spi;
    int fd;
    uint8_t index;
    uint8_t status;
} machine_spi_obj_t;

static bool spi_used[3];

/******************************************************************************/
// MicroPython bindings for generic machine.SPI

STATIC void machine_spi_obj_check(machine_spi_obj_t *self) {
    if (self->status == 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The SPI object has been deleted"));
    }
}

STATIC void mp_machine_spi_transfer(mp_obj_t *self_in, size_t src_len, const void *src, size_t dest_len, void *dest) {
    machine_spi_obj_t *self = (machine_spi_obj_t *)self_in;
    machine_spi_obj_check(self);
    struct rt_spi_priv_data *priv_data = mp_local_alloc(sizeof(struct rt_spi_priv_data));
    priv_data->send_buf = src;
    priv_data->send_length = src_len;
    priv_data->recv_buf = dest;
    priv_data->recv_length = dest_len;

    if (dest == NULL) {
        struct rt_spi_configuration cfg = {
            .mode = DWENUM_SPI_TX,
            .data_width = self->spi.bits,
            .max_hz = self->spi.baud,
        };
        ioctl(self->fd, RT_SPI_DEV_CTRL_CONFIG, &cfg);
        write(self->fd, src, src_len);
    } else if ((src == NULL) && (dest != NULL)) {
        struct rt_spi_configuration cfg = {
            .mode = DWENUM_SPI_RX,
            .data_width = self->spi.bits,
            .max_hz = self->spi.baud,
        };
        ioctl(self->fd, RT_SPI_DEV_CTRL_CONFIG, &cfg);
        read(self->fd, dest, dest_len);
    } else {
        struct rt_spi_configuration cfg = {
            .mode = DWENUM_SPI_EEPROM,
            .data_width = self->spi.bits,
            .max_hz = self->spi.baud,
        };
        ioctl(self->fd, RT_SPI_DEV_CTRL_CONFIG, &cfg);
        ioctl(self->fd, RT_SPI_DEV_CTRL_RW, priv_data);
    }

    mp_local_free(priv_data);
}

STATIC mp_obj_t mp_machine_spi_read(size_t n_args, const mp_obj_t *args) {
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[1]));
    memset(vstr.buf, n_args == 3 ? mp_obj_get_int(args[2]) : 0, vstr.len);
    mp_machine_spi_transfer(args[0], 0, NULL, vstr.len, vstr.buf);
    return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_spi_read_obj, 2, 3, mp_machine_spi_read);

STATIC mp_obj_t mp_machine_spi_readinto(size_t n_args, const mp_obj_t *args) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);
    memset(bufinfo.buf, n_args == 3 ? mp_obj_get_int(args[2]) : 0, bufinfo.len);
    mp_machine_spi_transfer(args[0], 0, NULL, bufinfo.len, bufinfo.buf);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_spi_readinto_obj, 2, 3, mp_machine_spi_readinto);

STATIC mp_obj_t mp_machine_spi_write(mp_obj_t self, mp_obj_t wr_buf) {
    mp_buffer_info_t src;
    mp_get_buffer_raise(wr_buf, &src, MP_BUFFER_READ);
    mp_machine_spi_transfer(self, src.len, (const uint8_t *)src.buf, 0, NULL);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_machine_spi_write_obj, mp_machine_spi_write);

STATIC mp_obj_t mp_machine_spi_write_readinto(mp_obj_t self, mp_obj_t wr_buf, mp_obj_t rd_buf) {
    mp_buffer_info_t src;
    mp_get_buffer_raise(wr_buf, &src, MP_BUFFER_READ);
    mp_buffer_info_t dest;
    mp_get_buffer_raise(rd_buf, &dest, MP_BUFFER_WRITE);
    // if (src.len != dest.len) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("buffers must be the same length"));
    // }
    mp_machine_spi_transfer(self, src.len, src.buf, dest.len, dest.buf);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_machine_spi_write_readinto_obj, mp_machine_spi_write_readinto);

STATIC void machine_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_spi_obj_check(self);
    mp_printf(print, "SPI %u: baudrate=%u, polarity=%u, phase=%u, bits=%u",
        self->index, self->spi.baud, self->spi.polarity, self->spi.phase, self->spi.bits);
}

STATIC void machine_spi_init_helper(machine_spi_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, { .u_int = 1000000 } },
        { MP_QSTR_polarity, MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_phase, MP_ARG_INT, { .u_int = 0 } },
        { MP_QSTR_bits, MP_ARG_INT, { .u_int = 8 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->spi.baud = args[ARG_baudrate].u_int;
    self->spi.polarity = args[ARG_polarity].u_int > 0 ? 1 : 0;
    self->spi.phase = args[ARG_phase].u_int > 0 ? 1 : 0;
    self->spi.bits = args[ARG_bits].u_int;

    if (self->spi.bits != 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid spi bits"));
    }
}

STATIC mp_obj_t machine_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 5, true);

    int index = mp_obj_get_int(args[0]);
    if (index < 0 || index > 2) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid spi number"));
    }
    if (spi_used[index]) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("SPI %u busy"), index);
    }

    char dev_name[16] = "/dev/spi0";
    dev_name[8] = '0' + index;
    int fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        mp_raise_OSError_with_filename(errno, dev_name);
    }

    machine_spi_obj_t *self = m_new_obj_with_finaliser(machine_spi_obj_t);
    self->base.type = &machine_spi_type;
    self->index = index;
    self->fd = fd;
    self->status = 1;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_spi_init_helper(self, n_args - 1, args + 1, &kw_args);
    self->status = 2;
    spi_used[index] = 1;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_spi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_spi_obj_check(args[0]);
    machine_spi_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_spi_init_obj, 1, machine_spi_init);

STATIC mp_obj_t machine_spi_deinit(mp_obj_t self_in) {
    machine_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_spi_obj_check(self);
    close(self->fd);
    if (self->status == 2)
        spi_used[self->index] = 0;
    self->status = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_spi_deinit_obj, machine_spi_deinit);

STATIC const mp_rom_map_elem_t machine_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },
};
MP_DEFINE_CONST_DICT(mp_machine_spi_locals_dict, machine_spi_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_spi_make_new,
    print, machine_spi_print,
    locals_dict, &mp_machine_spi_locals_dict);
