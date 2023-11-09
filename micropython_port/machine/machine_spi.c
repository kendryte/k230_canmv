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

/******************************************************************************/
// MicroPython bindings for generic machine.SPI

STATIC mp_obj_t machine_spi_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    spi_p->init(s, n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_spi_init_obj, 1, machine_spi_init);

STATIC mp_obj_t machine_spi_deinit(mp_obj_t self) {
    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(self);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    if (spi_p->deinit != NULL) {
        spi_p->deinit(s);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_spi_deinit_obj, machine_spi_deinit);

STATIC void mp_machine_spi_transfer(mp_obj_t self, size_t src_len, const void *src, size_t dest_len, void *dest) {
    mp_obj_base_t *s = (mp_obj_base_t *)MP_OBJ_TO_PTR(self);
    mp_machine_spi_p_t *spi_p = (mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(s->type, protocol);
    spi_p->transfer(s, src_len, src, dest_len, dest);
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

STATIC const mp_rom_map_elem_t machine_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_spi_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_spi_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_machine_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_machine_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_machine_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto), MP_ROM_PTR(&mp_machine_spi_write_readinto_obj) },
};
MP_DEFINE_CONST_DICT(mp_machine_spi_locals_dict, machine_spi_locals_dict_table);


/******************************************************************************/
// Implementation of soft SPI



STATIC void mp_machine_soft_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_machine_soft_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SoftSPI(id=%u,baudrate=%u, polarity=%u, phase=%u, bits=%u)",
        self->fd, self->spi.baud, self->spi.polarity, self->spi.phase, self->spi.bits);
}

STATIC mp_obj_t mp_machine_soft_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 20} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // create new object
    mp_machine_soft_spi_obj_t *self = mp_obj_malloc(mp_machine_soft_spi_obj_t, &machine_spi_type);

    // set parameters
    int id = args[ARG_id].u_int;
    switch (id)
    {
        case 0:
            self -> fd = open(SPI0_DEVICE_NAME, O_RDWR);
            break;

        case 1:
            self -> fd = open(SPI1_DEVICE_NAME, O_RDWR);
            break;

        case 2:
            self -> fd = open(SPI2_DEVICE_NAME, O_RDWR);
            break;
                                
        default:
            break;
    }
    if (self->fd < 0){
        mp_raise_OSError_with_filename(errno, SPI_DEVICE_NAME);     
        mp_raise_ValueError(MP_ERROR_TEXT("invalid id"));
    }

    self->spi.baud = args[ARG_baudrate].u_int;
    self->spi.polarity = args[ARG_polarity].u_int;
    self->spi.phase = args[ARG_phase].u_int;
    self->spi.bits = args[ARG_bits].u_int;
    if (args[ARG_bits].u_int != 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("bits must be 8"));
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC void mp_machine_soft_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_machine_soft_spi_obj_t *self = (mp_machine_soft_spi_obj_t *)self_in;

    enum {ARG_id, ARG_baudrate, ARG_polarity, ARG_phase,ARG_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_polarity, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_phase, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_bits,  MP_ARG_INT, {.u_int = 8} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set parameters
    int id = args[ARG_id].u_int;
    switch (id)
    {
        case 0:
            self -> fd = open(SPI0_DEVICE_NAME, O_RDWR);
            break;

        case 1:
            self -> fd = open(SPI1_DEVICE_NAME, O_RDWR);
            break;

        case 2:
            self -> fd = open(SPI2_DEVICE_NAME, O_RDWR);
            break;
                                
        default:
            break;
    }
    if (self->fd < 0){
        mp_raise_OSError_with_filename(errno, SPI_DEVICE_NAME);     
        mp_raise_ValueError(MP_ERROR_TEXT("invalid id"));
    }

    if (args[ARG_baudrate].u_int != -1) {
        self->spi.baud = args[ARG_baudrate].u_int;
    }
    if (args[ARG_polarity].u_int != -1) {
        self->spi.polarity = args[ARG_polarity].u_int;
    }
    if (args[ARG_phase].u_int != -1) {
        self->spi.phase = args[ARG_phase].u_int;
    }
    if (args[ARG_phase].u_int != -1) {
        self->spi.bits = args[ARG_phase].u_int;
    }
    
}

STATIC void mp_machine_soft_spi_deinit(mp_obj_base_t *self_in) {
    mp_machine_soft_spi_obj_t *self = (mp_machine_soft_spi_obj_t *)self_in;
    
    close(self->fd);
    self->fd = -1;
}

STATIC void mp_machine_soft_spi_transfer(mp_obj_base_t *self_in, size_t src_len, const uint8_t *src, size_t dest_len, uint8_t *dest) {
    mp_machine_soft_spi_obj_t *self = (mp_machine_soft_spi_obj_t *)self_in;
    struct rt_spi_priv_data *priv_data = mp_local_alloc(sizeof(struct rt_spi_priv_data));
    priv_data->send_buf = src;
    priv_data->send_length = src_len;
    priv_data->recv_buf = dest;
    priv_data->recv_length = dest_len;

    if(dest == NULL){ 
        struct rt_spi_configuration cfg = {
            .mode = DWENUM_SPI_TX,
            .data_width = self->spi.bits,
            .max_hz = self->spi.baud,
        };
        ioctl(self->fd, RT_SPI_DEV_CTRL_CONFIG, &cfg);
        write(self->fd, src, src_len);
    }else if((src == NULL)&&(dest != NULL)){
        struct rt_spi_configuration cfg = {
            .mode = DWENUM_SPI_RX,
            .data_width = self->spi.bits,
            .max_hz = self->spi.baud,
        };
        ioctl(self->fd, RT_SPI_DEV_CTRL_CONFIG, &cfg);
        read(self->fd, dest, dest_len);
    }else{
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

const mp_machine_spi_p_t mp_machine_soft_spi_p = {
    .init = mp_machine_soft_spi_init,
    .deinit = mp_machine_soft_spi_deinit,
    .transfer = mp_machine_soft_spi_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SoftSPI,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_soft_spi_make_new,
    print, mp_machine_soft_spi_print,
    protocol, &mp_machine_soft_spi_p,
    locals_dict, &mp_machine_spi_locals_dict
    );

