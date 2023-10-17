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
#ifndef MICROPY_INCLUDED_EXTMOD_MACHINE_I2C_H
#define MICROPY_INCLUDED_EXTMOD_MACHINE_I2C_H

#include "py/obj.h"
#include "py/mphal.h"
// #include <rtdevice.h>
// #include <rtthread.h>

#define I2C_DEVICE_NAME     "/dev/i2c"
#define I2C0_DEVICE_NAME     "/dev/i2c0"
#define I2C1_DEVICE_NAME     "/dev/i2c1"
#define I2C2_DEVICE_NAME     "/dev/i2c2"
#define I2C3_DEVICE_NAME     "/dev/i2c3"
#define I2C4_DEVICE_NAME     "/dev/i2c4"
#define I2C_CHANNEL_MAX     5

#define RT_I2C_DEV_CTRL_10BIT        (0x800 + 0x01)
#define RT_I2C_DEV_CTRL_ADDR         (0x800 + 0x02)
#define RT_I2C_DEV_CTRL_TIMEOUT      (0x800 + 0x03)
#define RT_I2C_DEV_CTRL_RW           (0x800 + 0x04)
#define RT_I2C_DEV_CTRL_CLK          (0x800 + 0x05)
#define RT_I2C_DEV_CTRL_7BIT         (0x800 + 0x06)

#define MP_MACHINE_I2C_FLAG_WRITE    0x0000
#define MP_MACHINE_I2C_FLAG_READ    (1u << 0)
// #define MP_MACHINE_I2C_FLAG_STOP    0x8000
#define MP_MACHINE_I2C_FLAG_STOP    0x0

extern const mp_obj_type_t machine_i2c_type;

typedef struct i2c_msg
{
    uint16_t addr;
    uint16_t flags;
    uint16_t len;
    uint8_t  *buf;
}i2c_msg_t;

typedef struct i2c_priv_data
{
    i2c_msg_t  *msgs;
    size_t  number;
}i2c_priv_data_t;

// Temporary support for legacy construction of SoftI2C via I2C type.
#define MP_MACHINE_I2C_CHECK_FOR_LEGACY_SOFTI2C_CONSTRUCTION(n_args, n_kw, all_args) \
    do { \
        if (n_args == 0 || all_args[0] == MP_OBJ_NEW_SMALL_INT(-1)) { \
            mp_print_str(MICROPY_ERROR_PRINTER, "Warning: I2C(-1, ...) is deprecated, use SoftI2C(...) instead\n"); \
            if (n_args != 0) { \
                --n_args; \
                ++all_args; \
            } \
            return MP_OBJ_TYPE_GET_SLOT(&mp_machine_soft_i2c_type, make_new)(&mp_machine_soft_i2c_type, n_args, n_kw, all_args); \
        } \
    } while (0)

// #define MP_MACHINE_I2C_FLAG_READ (0x01) // if not set then it's a write
// #define MP_MACHINE_I2C_FLAG_STOP (0x02)

#if MICROPY_PY_MACHINE_I2C_TRANSFER_WRITE1
// If set, the first mp_machine_i2c_buf_t in a transfer is a write.
#define MP_MACHINE_I2C_FLAG_WRITE1 (0x04)
#endif

typedef struct _mp_machine_i2c_buf_t {
    size_t len;
    uint8_t *buf;
} mp_machine_i2c_buf_t;

// I2C protocol
// - init must be non-NULL
// - start/stop/read/write can be NULL, meaning operation is not supported
// - transfer must be non-NULL
// - transfer_single only needs to be set if transfer=mp_machine_i2c_transfer_adaptor
typedef struct _mp_machine_i2c_p_t {
    #if MICROPY_PY_MACHINE_I2C_TRANSFER_WRITE1
    bool transfer_supports_write1;
    #endif
    void (*init)(mp_obj_base_t *obj, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
    int (*start)(mp_obj_base_t *obj);
    int (*stop)(mp_obj_base_t *obj);
    int (*read)(mp_obj_base_t *obj, uint8_t *dest, size_t len, bool nack);
    int (*write)(mp_obj_base_t *obj, const uint8_t *src, size_t len);
    int (*transfer)(mp_obj_base_t *obj, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags);
    int (*transfer_single)(mp_obj_base_t *obj, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags);
} mp_machine_i2c_p_t;

typedef struct _mp_machine_soft_i2c_obj_t {
    mp_obj_base_t base;
    uint32_t freq;
    uint32_t addr_size;
    int fd;
    // rt_device_t canmv_i2c;
    uint8_t used[I2C_CHANNEL_MAX];
} mp_machine_soft_i2c_obj_t;

extern const mp_obj_type_t mp_machine_soft_i2c_type;
extern const mp_obj_dict_t mp_machine_i2c_locals_dict;

int mp_machine_i2c_transfer_adaptor(mp_obj_base_t *self, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags);
int mp_machine_soft_i2c_transfer(mp_obj_base_t *self, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags);

#endif // MICROPY_INCLUDED_EXTMOD_MACHINE_I2C_H
