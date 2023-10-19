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

#ifndef DRV_GPIO_H__
#define DRV_GPIO_H__
#define GPIO_MAX_NUM                64
#define IRQN_GPIO0_INTERRUPT      32

/* k230 gpio register table */
#define DATA_OUTPUT         0x0
#define DIRECTION           0x04
#define DATA_SOURCE         0x08
#define INT_ENABLE          0x30
#define INT_MASK            0x34
#define INT_TYPE_LEVEL      0x38
#define INT_POLARITY        0x3c
#define INT_STATUS          0x40
#define INT_STATUS_RAW      0x44
#define INT_DEBOUNCE        0x48
#define INT_CLEAR           0x4c
#define DATA_INPUT          0x50
#define VER_ID_CODE         0x64
#define INT_BOTHEDGE        0x68

#define KD_GPIO_HIGH                1
#define KD_GPIO_LOW                 0
#define KD_GPIO_IRQ_DISABLE      0x00
#define KD_GPIO_IRQ_ENABLE       0x01

/* ioctl */

#define	KD_GPIO_DM_OUTPUT           _IOW('G', 0, int)
#define	KD_GPIO_DM_INPUT            _IOW('G', 1, int)
#define	KD_GPIO_DM_INPUT_PULL_UP    _IOW('G', 2, int)
#define	KD_GPIO_DM_INPUT_PULL_DOWN  _IOW('G', 3, int)
#define	KD_GPIO_WRITE_LOW           _IOW('G', 4, int)
#define	KD_GPIO_WRITE_HIGH          _IOW('G', 5, int)

#define	KD_GPIO_PE_RISING           _IOW('G', 7, int)
#define	KD_GPIO_PE_FALLING          _IOW('G', 8, int)
#define	KD_GPIO_PE_BOTH             _IOW('G', 9, int)
#define	KD_GPIO_PE_HIGH             _IOW('G', 10, int)
#define	KD_GPIO_PE_LOW              _IOW('G', 11, int)

#define KD_GPIO_READ_VALUE       	_IOW('G', 12, int)

#define	KD_GPIO_ENABLE_IRQ           _IOW('G', 13, int)
#define	KD_GPIO_DISABLE_IRQ          _IOW('G', 14, int)
#define	KD_GPIO_ATTACH_IRQ           _IOW('G', 15, int)
#define	KD_GPIO_DETACH_IRQ           _IOW('G', 16, int)


// #define KD_GPIO_PE_RISING           _IOW('G', 17, int)
// #define KD_GPIO_PE_FALLING          _IOW('G', 18, int)
// #define KD_GPIO_PE_BOTH             _IOW('G', 19, int)
// #define KD_GPIO_PE_HIGH             _IOW('G', 20, int)
// #define KD_GPIO_PE_LOW              _IOW('G', 21, int)


typedef enum _gpio_pin_edge
{
    GPIO_PE_RISING,
    GPIO_PE_FALLING,
    GPIO_PE_BOTH,
    GPIO_PE_HIGH,
    GPIO_PE_LOW,
} gpio_pin_edge_t;

typedef enum _gpio_drive_mode
{
    GPIO_DM_OUTPUT,
    GPIO_DM_INPUT,
    GPIO_DM_INPUT_PULL_UP,
    GPIO_DM_INPUT_PULL_DOWN,
} gpio_drive_mode_t;

typedef enum _gpio_pin_value
{
    GPIO_PV_LOW,
    GPIO_PV_HIGH
} gpio_pin_value_t;

int rt_hw_pin_init(void);

rt_err_t kd_pin_irq_enable(rt_base_t pin, rt_uint32_t enabled);
rt_err_t kd_pin_detach_irq(rt_int32_t pin);
rt_err_t kd_pin_attach_irq(rt_int32_t pin,rt_uint32_t mode, void (*hdr)(void *args), void *args);
void kd_pin_write(rt_base_t pin, rt_base_t value);
void kd_pin_mode(rt_base_t pin, rt_base_t mode);
int kd_pin_read(rt_base_t pin);

#endif
