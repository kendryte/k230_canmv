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
#ifndef MICROPY_INCLUDED_EXTMOD_MACHINE_SPI_H
#define MICROPY_INCLUDED_EXTMOD_MACHINE_SPI_H

#include "py/obj.h"
#include "py/mphal.h"

#define SPI_DEVICE_NAME     "/dev/spi"
#define SPI0_DEVICE_NAME     "/dev/spi0"
#define SPI1_DEVICE_NAME     "/dev/spi1"
#define SPI2_DEVICE_NAME     "/dev/spi2"

#define RT_SPI_DEV_CTRL_CONFIG       (0xc00+ 0x01)
#define RT_SPI_DEV_CTRL_RW           (0xc00 + 0x02)
#define RT_SPI_DEV_CTRL_CLK          (0xc00 + 0x03)

struct rt_spi_priv_data {
    const void           *send_buf;
    size_t             send_length;
    void                 *recv_buf;
    size_t             recv_length;
};

struct rt_spi_configuration
{
    uint8_t mode;
    uint8_t data_width;
    uint16_t reserved;
    uint32_t max_hz;
};

typedef enum {
    DWENUM_SPI_TXRX = 0,
    DWENUM_SPI_TX   = 1,
    DWENUM_SPI_RX   = 2,
    DWENUM_SPI_EEPROM = 3
} DWENUM_SPI_TRANSFER_MODE;

enum {
    MP_SPI_IOCTL_INIT,
    MP_SPI_IOCTL_DEINIT,
};

typedef struct _mp_spi_proto_t {
    int (*ioctl)(void *self, uint32_t cmd);
    void (*transfer)(void *self, size_t src_len, const uint8_t *src, size_t dest_len, uint8_t *dest);
} mp_spi_proto_t;

typedef struct _mp_soft_spi_obj_t {
    uint32_t baud; 
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t reserver;
} mp_soft_spi_obj_t;

extern const mp_spi_proto_t mp_soft_spi_proto;

int mp_soft_spi_ioctl(void *self, uint32_t cmd);
void mp_soft_spi_transfer(void *self, size_t src_len, const uint8_t *src, size_t dest_len, uint8_t *dest);


// Temporary support for legacy construction of SoftSPI via SPI type.
#define MP_MACHINE_SPI_CHECK_FOR_LEGACY_SOFTSPI_CONSTRUCTION(n_args, n_kw, all_args) \
    do { \
        if (n_args == 0 || all_args[0] == MP_OBJ_NEW_SMALL_INT(-1)) { \
            mp_print_str(MICROPY_ERROR_PRINTER, "Warning: SPI(-1, ...) is deprecated, use SoftSPI(...) instead\n"); \
            if (n_args != 0) { \
                --n_args; \
                ++all_args; \
            } \
            return MP_OBJ_TYPE_GET_SLOT(&mp_machine_soft_spi_type, make_new)(&mp_machine_soft_spi_type, n_args, n_kw, all_args); \
        } \
    } while (0)

// SPI protocol
typedef struct _mp_machine_spi_p_t {
    void (*init)(mp_obj_base_t *obj, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
    void (*deinit)(mp_obj_base_t *obj); // can be NULL
    void (*transfer)(mp_obj_base_t *obj, size_t src_len, const uint8_t *src, size_t dest_len, uint8_t *dest);
} mp_machine_spi_p_t;

typedef struct _mp_machine_soft_spi_obj_t {
    mp_obj_base_t base;
    mp_soft_spi_obj_t spi;
    uint32_t fd;
} mp_machine_soft_spi_obj_t;

extern const mp_machine_spi_p_t mp_machine_soft_spi_p;
extern const mp_obj_type_t machine_spi_type;
extern const mp_obj_dict_t mp_machine_spi_locals_dict;

mp_obj_t mp_machine_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_spi_read_obj);
MP_DECLARE_CONST_FUN_OBJ_VAR_BETWEEN(mp_machine_spi_readinto_obj);
MP_DECLARE_CONST_FUN_OBJ_2(mp_machine_spi_write_obj);
MP_DECLARE_CONST_FUN_OBJ_3(mp_machine_spi_write_readinto_obj);

#endif // MICROPY_INCLUDED_EXTMOD_MACHINE_SPI_H
