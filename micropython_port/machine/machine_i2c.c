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

#define I2C_CHANNEL_MAX 5

#define RT_I2C_DEV_CTRL_10BIT (0x800 + 0x01)
#define RT_I2C_DEV_CTRL_ADDR (0x800 + 0x02)
#define RT_I2C_DEV_CTRL_TIMEOUT (0x800 + 0x03)
#define RT_I2C_DEV_CTRL_RW (0x800 + 0x04)
#define RT_I2C_DEV_CTRL_CLK (0x800 + 0x05)
#define RT_I2C_DEV_CTRL_7BIT (0x800 + 0x06)

#define MP_MACHINE_I2C_FLAG_WRITE 0x0000
#define MP_MACHINE_I2C_FLAG_READ (1u << 0)
// #define MP_MACHINE_I2C_FLAG_STOP    0x8000
#define MP_MACHINE_I2C_FLAG_STOP 0x0

typedef struct {
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t *buf;
} i2c_msg_t;

typedef struct {
    i2c_msg_t *msgs;
    size_t number;
} i2c_priv_data_t;

typedef struct {
    size_t len;
    uint8_t *buf;
} mp_machine_i2c_buf_t;

// I2C protocol
// - init must be non-NULL
// - start/stop/read/write can be NULL, meaning operation is not supported
// - transfer must be non-NULL
// - transfer_single only needs to be set if transfer=mp_machine_i2c_transfer_adaptor
typedef struct {
    #if MICROPY_PY_MACHINE_I2C_TRANSFER_WRITE1
    bool transfer_supports_write1;
    #endif
    void (*init)(mp_obj_base_t * obj, size_t n_args, const mp_obj_t * pos_args, mp_map_t * kw_args);
    int (*start)(mp_obj_base_t * obj);
    int (*stop)(mp_obj_base_t * obj);
    int (*read)(mp_obj_base_t * obj, uint8_t * dest, size_t len, bool nack);
    int (*write)(mp_obj_base_t * obj, const uint8_t * src, size_t len);
    int (*transfer)(mp_obj_base_t * obj, uint16_t addr, size_t n, mp_machine_i2c_buf_t * bufs, unsigned int flags);
    int (*transfer_single)(mp_obj_base_t * obj, uint16_t addr, size_t len, uint8_t * buf, unsigned int flags);
} mp_machine_i2c_p_t;

typedef struct {
    mp_obj_base_t base;
    int fd;
    uint32_t freq;
    uint8_t index;
    uint8_t status;
} machine_i2c_obj_t;

static bool i2c_used[I2C_CHANNEL_MAX];

STATIC void mp_hal_i2c_init(machine_i2c_obj_t *self, uint32_t freq) {
    ioctl(self->fd, RT_I2C_DEV_CTRL_CLK, &freq);
    ioctl(self->fd, RT_I2C_DEV_CTRL_7BIT, NULL);
}

STATIC void machine_i2c_obj_check(machine_i2c_obj_t *self) {
    if (self->status == 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The I2C object has been deleted"));
    }
}

// return value:
//  >=0 - success; for read it's 0, for write it's number of acks received
//   <0 - error, with errno being the negative of the return value
int mp_machine_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;
    machine_i2c_obj_check(self);
    int ret = 0, i;

    i2c_priv_data_t *data = mp_local_alloc(sizeof(i2c_priv_data_t));
    data->msgs = mp_local_alloc(n * sizeof(i2c_msg_t));
    data->number = n;
    for (i = 0; i < n; i++) {
        if (((flags & 0x1) == MP_MACHINE_I2C_FLAG_READ) && (n > 1) && (i == 0)) {
            data->msgs[i].flags = MP_MACHINE_I2C_FLAG_WRITE;
        } else {
            data->msgs[i].flags = flags;
        }
        data->msgs[i].addr = addr;
        data->msgs[i].buf = bufs->buf;
        data->msgs[i].len = bufs->len;
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
    mp_machine_i2c_buf_t buf = { .len = len, .buf = dest };
    unsigned int flags = MP_MACHINE_I2C_FLAG_READ | (stop ? MP_MACHINE_I2C_FLAG_STOP : 0);
    return i2c_p->transfer(self, addr, 1, &buf, flags);
}

STATIC int mp_machine_i2c_writeto(mp_obj_base_t *self, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    mp_machine_i2c_buf_t buf = { .len = len, .buf = (uint8_t *)src };
    unsigned int flags = stop ? MP_MACHINE_I2C_FLAG_STOP : 0;
    return i2c_p->transfer(self, addr, 1, &buf, flags);
}

/******************************************************************************/
// MicroPython bindings for generic machine.I2C

STATIC mp_obj_t machine_i2c_scan(mp_obj_t self_in) {
    mp_obj_base_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint8_t buf[256] = { 0 };
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
    // The I2C transfer function may support the MP_MACHINE_I2C_FLAG_WRITE1 option
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    // Create partial write and read buffers
    mp_machine_i2c_buf_t bufs[2] = {
        { .len = memaddr_len, .buf = memaddr_buf },
        { .len = len, .buf = buf },
    };

    // Do write+read I2C transfer
    return i2c_p->transfer(self, addr, 2, bufs,
        MP_MACHINE_I2C_FLAG_READ | MP_MACHINE_I2C_FLAG_STOP);
}

STATIC int write_mem(mp_obj_t self_in, uint16_t addr, uint32_t memaddr, uint8_t mem_size, const uint8_t *buf, size_t len) {
    mp_obj_base_t *self = (mp_obj_base_t *)MP_OBJ_TO_PTR(self_in);

    // Create buffer with memory address
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], memaddr, mem_size);
    int i, ret = 0;

    mp_machine_i2c_buf_t *bufs = mp_local_alloc(sizeof(mp_machine_i2c_buf_t));
    bufs->buf = mp_local_alloc((memaddr_len + len) * sizeof(uint8_t *));

    // Create partial write buffers
    // mp_machine_i2c_buf_t bufs[2] = {
    //     {.len = memaddr_len, .buf = memaddr_buf},
    //     {.len = len, .buf = (uint8_t *)buf},
    // };
    // mp_machine_i2c_buf_t bufs[1];
    bufs[0].len = memaddr_len + len;
    for (i = 0; i < memaddr_len; i++) {
        bufs[0].buf[i] = memaddr_buf[i];
    }
    for (i = 0; i < len; i++) {
        bufs[0].buf[memaddr_len + i] = buf[i];
    }

    // Do I2C transfer
    mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(self->type, protocol);
    ret = i2c_p->transfer(self, addr, 1, bufs, MP_MACHINE_I2C_FLAG_STOP);

    mp_local_free(bufs);
    mp_local_free(bufs->buf);

    return ret;
}

STATIC const mp_arg_t machine_i2c_mem_allowed_args[] = {
    { MP_QSTR_addr, MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
    { MP_QSTR_memaddr, MP_ARG_REQUIRED | MP_ARG_INT, { .u_int = 0 } },
    { MP_QSTR_arg, MP_ARG_REQUIRED | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
    { MP_QSTR_mem_size, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 8 } },
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

/******************************************************************************/
// Implementation of soft I2C

STATIC void machine_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "IIC %u: freq=%u", self->index, self->freq);
}

STATIC void machine_i2c_init_helper(machine_i2c_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_freq };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_freq, MP_ARG_INT, { .u_int = 400000 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->freq = args[ARG_freq].u_int;
    if (self->freq < 100 || self->freq > 10000000) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid freq"));
    }

    mp_hal_i2c_init(self, args[ARG_freq].u_int);
}

STATIC mp_obj_t machine_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 2, true);

    int index = mp_obj_get_int(args[0]);
    if (index < 0 || index >= I2C_CHANNEL_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid i2c number"));
    }
    if (i2c_used[index]) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("I2C %u busy"), index);
    }

    char dev_name[16] = "/dev/i2c0";
    dev_name[8] = '0' + index;
    int fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        mp_raise_OSError_with_filename(errno, dev_name);
    }

    machine_i2c_obj_t *self = m_new_obj_with_finaliser(machine_i2c_obj_t);
    self->base.type = &machine_i2c_type;
    self->index = index;
    self->fd = fd;
    self->status = 1;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_i2c_init_helper(self, n_args - 1, args + 1, &kw_args);
    self->status = 2;
    i2c_used[index] = 1;

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_i2c_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_i2c_obj_check(args[0]);
    machine_i2c_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_i2c_init_obj, 1, machine_i2c_init);

STATIC mp_obj_t machine_i2c_deinit(mp_obj_t self_in) {
    machine_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_i2c_obj_check(self);
    close(self->fd);
    if (self->status == 2)
        i2c_used[self->index] = 0;
    self->status = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_i2c_deinit_obj, machine_i2c_deinit);

int mp_machine_i2c_read(mp_obj_base_t *self_in, uint8_t *dest, size_t len, bool nack) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;
    machine_i2c_obj_check(self);

    int ret = read(self->fd, dest, len);

    return ret;
}

int mp_machine_i2c_write(mp_obj_base_t *self_in, const uint8_t *src, size_t len) {
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)self_in;
    machine_i2c_obj_check(self);

    int ret = write(self->fd, src, len);

    return ret;
}

STATIC const mp_rom_map_elem_t machine_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_i2c_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_i2c_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_i2c_deinit_obj) },
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

STATIC const mp_machine_i2c_p_t mp_machine_i2c_p = {
    .read = mp_machine_i2c_read,
    .write = mp_machine_i2c_write,
    .transfer = mp_machine_i2c_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, machine_i2c_make_new,
    print, machine_i2c_print,
    protocol, &mp_machine_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict);
