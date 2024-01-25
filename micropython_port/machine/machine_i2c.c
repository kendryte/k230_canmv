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
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "sys/ioctl.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "machine_i2c.h"
#include "py/obj.h"

#define SOFT_I2C_DEFAULT_TIMEOUT_US (50000) // 50ms

typedef mp_machine_soft_i2c_obj_t machine_i2c_obj_t;

STATIC void mp_hal_i2c_init(machine_i2c_obj_t *self, uint32_t freq) {
    ioctl(self->fd, RT_I2C_DEV_CTRL_CLK, &freq);
    if(self->addr_size == 10)
        ioctl(self->fd, RT_I2C_DEV_CTRL_10BIT, NULL);
    else
        ioctl(self->fd, RT_I2C_DEV_CTRL_7BIT, NULL);

}

// return value:
//  >=0 - success; for read it's 0, for write it's number of acks received
//   <0 - error, with errno being the negative of the return value
int mp_machine_soft_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;
    int ret = 0,i;

    i2c_priv_data_t *data = mp_local_alloc(sizeof(i2c_priv_data_t));
    data->msgs = mp_local_alloc(n * sizeof(i2c_msg_t));
    data->number = n;
    for (i = 0;i < n; i++) {
        if(((flags & 0x1) == MP_MACHINE_I2C_FLAG_READ)&&(n > 1)&&(i == 0)){
            data->msgs[i].flags = MP_MACHINE_I2C_FLAG_WRITE;
        }else{
            data->msgs[i].flags = flags;
        }
            data->msgs[i].addr  = addr;
            data->msgs[i].buf   = bufs->buf;
            data->msgs[i].len   = bufs->len;
            ++bufs;
    }

    ret = ioctl(self->fd, RT_I2C_DEV_CTRL_RW, data);

    mp_local_free(data);
    mp_local_free(data->msgs);

    return ret;
}


/******************************************************************************/
// Generic helper functions

STATIC int mp_machine_i2c_readfrom(mp_obj_base_t *self, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    mp_machine_i2c_buf_t buf = {.len = len, .buf = dest};
    unsigned int flags = MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    return i2c_p->transfer(self, addr, 1, &buf, flags);
}

STATIC int mp_machine_i2c_writeto(mp_obj_base_t *self, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    mp_machine_i2c_buf_t buf = {.len = len, .buf = (uint8_t *)src};
    unsigned int flags = stop ? MP_MACHINE_I2C_FLAG_STOP : 0;
    return i2c_p->transfer(self, addr, 1, &buf, flags);
}

/******************************************************************************/
// MicroPython bindings for generic machine.I2C

STATIC mp_obj_t machine_i2c_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    if (i2c_p->init == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C operation not supported"));
    }
    i2c_p->init(self, n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_init_obj, 1, machine_i2c_init);

STATIC mp_obj_t machine_i2c_scan(mp_obj_t self_in) {
    mp_obj_base_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint8_t buf[256]={0};
    // 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
    for (int addr = 0x08; addr < 0x78; ++addr) {
        // int ret = mp_machine_i2c_writeto(self, addr, NULL, 0, true);
        int ret = mp_machine_i2c_readfrom(self, addr, buf, 1, true);
        if (ret == 0) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
        #ifdef MICROPY_EVENT_POLL_HOOK
        MICROPY_EVENT_POLL_HOOK
        #endif
    }
    return list;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_scan_obj, machine_i2c_scan);

STATIC mp_obj_t machine_i2c_start(mp_obj_t self_in) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    if (i2c_p->start == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C operation not supported"));
    }
    int ret = i2c_p->start(self);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_start_obj, machine_i2c_start);

STATIC mp_obj_t machine_i2c_stop(mp_obj_t self_in) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    if (i2c_p->stop == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C operation not supported"));
    }
    int ret = i2c_p->stop(self);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_stop_obj, machine_i2c_stop);

STATIC mp_obj_t machine_i2c_readinto(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    if (i2c_p->read == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C operation not supported"));
    }

    // get the buffer to read into
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);

    // work out if we want to send a nack at the end
    bool nack = (n_args == 2) ? true : mp_obj_is_true(args[2]);

    // do the read
    int ret = i2c_p->read(self, bufinfo.buf, bufinfo.len, nack);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readinto_obj, 2, 3, machine_i2c_readinto);

STATIC mp_obj_t machine_i2c_write(mp_obj_t self_in, mp_obj_t buf_in) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    if (i2c_p->write == NULL) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("I2C operation not supported"));
    }

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    // do the write
    int ret = i2c_p->write(self, bufinfo.buf, bufinfo.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    // return number of acks received
    return MP_OBJ_NEW_SMALL_INT(ret);
}
MP_DEFINE_CONST_FUN_OBJ_2(machine_i2c_write_obj, machine_i2c_write);

STATIC mp_obj_t machine_i2c_readfrom(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[2]));
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = mp_machine_i2c_readfrom(self, addr, (uint8_t *)vstr.buf, vstr.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_obj, 3, 4, machine_i2c_readfrom);

STATIC mp_obj_t machine_i2c_readfrom_into(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = mp_machine_i2c_readfrom(self, addr, bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_readfrom_into_obj, 3, 4, machine_i2c_readfrom_into);

STATIC mp_obj_t machine_i2c_writeto(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);
    int ret = mp_machine_i2c_writeto(self, addr, bufinfo.buf, bufinfo.len, stop);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    // return number of acks received
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_writeto_obj, 3, 4, machine_i2c_writeto);

STATIC mp_obj_t machine_i2c_writevto(size_t n_args, const mp_obj_t *args) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(args[0]);
    mp_int_t addr = mp_obj_get_int(args[1]);

    // Get the list of data buffer(s) to write
    size_t nitems;
    const mp_obj_t *items;
    mp_obj_get_array(args[2], &nitems, (mp_obj_t **)&items);

    // Get the stop argument
    bool stop = (n_args == 3) ? true : mp_obj_is_true(args[3]);

    // Extract all buffer data, skipping zero-length buffers
    size_t alloc = nitems == 0 ? 1 : nitems;
    size_t nbufs = 0;
    mp_machine_i2c_buf_t *bufs = mp_local_alloc(alloc * sizeof(mp_machine_i2c_buf_t));
    for (; nitems--; ++items) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(*items, &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len > 0) {
            bufs[nbufs].len = bufinfo.len;
            bufs[nbufs++].buf = bufinfo.buf;
        }
    }

    // Make sure there is at least one buffer, empty if needed
    if (nbufs == 0) {
        bufs[0].len = 0;
        bufs[0].buf = NULL;
        nbufs = 1;
    }

    // Do the I2C transfer
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    int ret = i2c_p->transfer(self, addr, nbufs, bufs, stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    mp_local_free(bufs);

    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    // Return number of acks received
    return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_writevto_obj, 3, 4, machine_i2c_writevto);

STATIC size_t fill_memaddr_buf(uint8_t *memaddr_buf, uint32_t memaddr, uint8_t mem_size) {
    size_t memaddr_len = 0;
    if ((mem_size & 7) != 0 || mem_size > 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid mem_size"));
    }
    for (int16_t i = mem_size - 8; i >= 0; i -= 8) {
        memaddr_buf[memaddr_len++] = memaddr >> i;
    }
    return memaddr_len;
}

STATIC int read_mem(mp_obj_t self_in, uint16_t addr, uint32_t memaddr, uint8_t mem_size, uint8_t *buf, size_t len) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);

    // Create buffer with memory address
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], memaddr, mem_size);
#if 1
    // The I2C transfer function may support the MP_MACHINE_I2C_FLAG_WRITE1 option
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    // if (i2c_p->transfer_supports_write1) {
        // Create partial write and read buffers
        mp_machine_i2c_buf_t bufs[2] = {
            {.len = memaddr_len, .buf = memaddr_buf},
            {.len = len, .buf = buf},
        };

        // Do write+read I2C transfer
        return i2c_p->transfer(self, addr, 2, bufs,
                MP_MACHINE_I2C_FLAG_READ | MP_MACHINE_I2C_FLAG_STOP);
    // }
#else
    int ret = mp_machine_i2c_writeto(self, addr, memaddr_buf, memaddr_len, false);
    if (ret != memaddr_len) {
        // must generate STOP
        mp_machine_i2c_writeto(self, addr, NULL, 0, true);
        return ret;
    }
    return mp_machine_i2c_readfrom(self, addr, buf, len, true);
#endif
}

STATIC int write_mem(mp_obj_t self_in, uint16_t addr, uint32_t memaddr, uint8_t mem_size, const uint8_t *buf, size_t len) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);

    // Create buffer with memory address
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], memaddr, mem_size);
    int i,ret = 0;

    mp_machine_i2c_buf_t *bufs = mp_local_alloc(sizeof(mp_machine_i2c_buf_t));
    bufs->buf = mp_local_alloc((memaddr_len + len) * sizeof(uint8_t *));

    // Create partial write buffers
    // mp_machine_i2c_buf_t bufs[2] = {
    //     {.len = memaddr_len, .buf = memaddr_buf},
    //     {.len = len, .buf = (uint8_t *)buf},
    // };
    // mp_machine_i2c_buf_t bufs[1];
    bufs[0].len = memaddr_len + len;
    for(i = 0;i < memaddr_len;i++)
        bufs[0].buf[i] = memaddr_buf[i];
    for(i = 0;i < len;i++) 
        bufs[0].buf[memaddr_len + i] = buf[i];

    // Do I2C transfer
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    ret = i2c_p->transfer(self, addr, 1, bufs, MP_MACHINE_I2C_FLAG_STOP);

    mp_local_free(bufs);
    mp_local_free(bufs->buf);

    return ret;
}

STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_addr,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_arg,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_mem_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
};

STATIC mp_obj_t machine_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_n, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // create the buffer to store data into
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(args[ARG_n].u_obj));

    // do the transfer
    int ret = read_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, (uint8_t *)vstr.buf, vstr.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    return mp_obj_new_bytes_from_vstr(&vstr);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_obj, 1, machine_i2c_readfrom_mem);


STATIC mp_obj_t machine_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // get the buffer to store data into
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);

    // do the transfer
    int ret = read_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, bufinfo.buf, bufinfo.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_readfrom_mem_into_obj, 1, machine_i2c_readfrom_mem_into);

STATIC mp_obj_t machine_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_addr, ARG_memaddr, ARG_buf, ARG_mem_size };
    mp_arg_val_t args[MP_ARRAY_SIZE(machine_i2c_mem_allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args,
        MP_ARRAY_SIZE(machine_i2c_mem_allowed_args), machine_i2c_mem_allowed_args, args);

    // get the buffer to write the data from
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);

    // do the transfer
    int ret = write_mem(pos_args[0], args[ARG_addr].u_int, args[ARG_memaddr].u_int,
        args[ARG_mem_size].u_int, bufinfo.buf, bufinfo.len);
    if (ret < 0) {
        mp_raise_OSError(-ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_writeto_mem_obj, 1, machine_i2c_writeto_mem);

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&machine_i2c_scan_obj) },

    // primitive I2C operations
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&machine_i2c_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_i2c_write_obj) },

    // standard bus operations
    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&machine_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into), MP_ROM_PTR(&machine_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&machine_i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_writevto), MP_ROM_PTR(&machine_i2c_writevto_obj) },

    // memory operations
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&machine_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&machine_i2c_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem), MP_ROM_PTR(&machine_i2c_writeto_mem_obj) },
};
MP_DEFINE_CONST_DICT(mp_machine_i2c_locals_dict, machine_i2c_locals_dict_table);



/******************************************************************************/
// Implementation of soft I2C



STATIC void mp_machine_soft_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_machine_soft_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "freq= %d , addr_size= %d , fd= %d",
        self->freq, self->addr_size, self->fd);
}

STATIC void mp_machine_soft_i2c_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq, ARG_addr_size };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_addr_size, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 7} },
    };

    mp_machine_soft_i2c_obj_t *self = (mp_machine_soft_i2c_obj_t *)self_in;
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->freq = args[ARG_freq].u_int;
    self->addr_size = args[ARG_addr_size].u_int;
    mp_hal_i2c_init(self, args[ARG_freq].u_int);
}

STATIC mp_obj_t mp_machine_soft_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    
    // create new soft I2C object
    machine_i2c_obj_t *self = mp_obj_malloc(machine_i2c_obj_t, &machine_i2c_type);
    mp_map_t kw_args;
    int channel;

    mp_arg_check_num(n_args, n_kw, 1, 4, true);

    if (mp_obj_is_int(args[0])) {
        channel = mp_obj_get_int(args[0]);
        if (channel < 0 || channel >= I2C_CHANNEL_MAX)
            mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
        if (self->used[channel])
            mp_raise_ValueError(MP_ERROR_TEXT("channel busy"));
    } else {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
    }

    switch (channel)
    {
        case 0:
            self -> fd = open(I2C0_DEVICE_NAME, O_RDWR);
            break;

        case 1:
            self -> fd = open(I2C1_DEVICE_NAME, O_RDWR);
            break;

        case 2:
            self -> fd = open(I2C2_DEVICE_NAME, O_RDWR);
            break;

        case 3:
            self -> fd = open(I2C3_DEVICE_NAME, O_RDWR);
            break;

        case 4:
            self -> fd = open(I2C4_DEVICE_NAME, O_RDWR);
            break;
                                
        default:
            break;
    }   
    if (self->fd < 0){
        mp_raise_OSError_with_filename(errno, I2C_DEVICE_NAME);     
        mp_raise_ValueError(MP_ERROR_TEXT("invalid channel"));
    }

    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    mp_machine_soft_i2c_init(&self->base, n_args - 1, args + 1, &kw_args);

    self->used[channel] = 1;

    return MP_OBJ_FROM_PTR(self);
}

int mp_machine_soft_i2c_read(mp_obj_base_t *self_in, uint8_t *dest, size_t len, bool nack) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;
    int ret = read(self->fd, dest, len);

    if(ret > 0)
        return ret;

    return 0; // success
}

int mp_machine_soft_i2c_write(mp_obj_base_t *self_in, const uint8_t *src, size_t len) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;

    int ret = write(self->fd, src, len);

    return ret;
}

STATIC const mp_machine_i2c_p_t mp_machine_soft_i2c_p = {
    .init = mp_machine_soft_i2c_init,
    .read = mp_machine_soft_i2c_read,
    .write = mp_machine_soft_i2c_write,
    .transfer = mp_machine_soft_i2c_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_SoftI2C,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_soft_i2c_make_new,
    print, mp_machine_soft_i2c_print,
    protocol, &mp_machine_soft_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict
    );


