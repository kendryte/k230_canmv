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
#include "machine_gpio.h"
#include "py/runtime.h"
#include "py/obj.h"

#define KD_GPIO_HIGH     1
#define KD_GPIO_LOW      0

/* ioctl */

#define device_name "/dev/gpio"
#define MAX_GPIO_NUM                63
#define	GPIO_DM_OUTPUT           _IOW('G', 0, int)
#define	GPIO_DM_INPUT            _IOW('G', 1, int)
#define	GPIO_DM_INPUT_PULL_UP    _IOW('G', 2, int)
#define	GPIO_DM_INPUT_PULL_DOWN  _IOW('G', 3, int)
#define	GPIO_WRITE_LOW           _IOW('G', 4, int)
#define	GPIO_WRITE_HIGH          _IOW('G', 5, int)

#define	KD_GPIO_PE_RISING           _IOW('G', 7, int)
#define	KD_GPIO_PE_FALLING          _IOW('G', 8, int)
#define	KD_GPIO_PE_BOTH             _IOW('G', 9, int)
#define	KD_GPIO_PE_HIGH             _IOW('G', 10, int)
#define	KD_GPIO_PE_LOW              _IOW('G', 11, int)

#define GPIO_READ_VALUE       	     _IOW('G', 12, int)
#define	KD_GPIO_ENABLE_IRQ           _IOW('G', 13, int)
#define	KD_GPIO_DISABLE_IRQ          _IOW('G', 14, int)
#define	KD_GPIO_ATTACH_IRQ           _IOW('G', 15, int)
#define	KD_GPIO_DETACH_IRQ           _IOW('G', 16, int)


enum {
    GPIO_DM_PULL_NONE = -1,
};

typedef enum _gpio_drive_mode
{
    GPIO_OUTPUT,
    GPIO_INPUT,
    GPIO_INPUT_PULL_UP,
    GPIO_INPUT_PULL_DOWN,
} gpio_drive_mode_t;

typedef struct kd_pin_mode
{
    unsigned short pin;     /* pin number, from 0 to 63 */
    unsigned short mode;    /* pin level status, 0 low level, 1 high level */
} pin_mode_t;

typedef enum _gpio_pin_edge
{
    GPIO_PE_NONE,
    GPIO_PE_FALLING,
    GPIO_PE_RISING,
    GPIO_PE_BOTH,
    GPIO_PE_LOW,
    GPIO_PE_HIGH = 8,
} gpio_pin_edge_t;

typedef struct _machine_gpio_obj_t {
    mp_obj_base_t base;
    int fd;
    pin_mode_t gpio;
    mp_obj_t callback;
    mp_obj_t arg;
} machine_gpio_obj_t;

machine_gpio_obj_t *overall_self;
static int gpio_fd = -1;

STATIC void sigroutine(int signo)
{
   switch (signo){
   case SIGUSR2:
        printf("Catch a signal -- SIGUSR2 \n");
        mp_sched_schedule(overall_self->callback,overall_self->arg);
        break;
   case SIGUSR1:
        printf("Catch a signal -- SIGUSR1 \n");
        break;
   }
}


STATIC void mp_machine_gpio_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_gpio_obj_t *self = self_in;
    mp_printf(print, 
        "[CANMV]GPIO:(%p; id=%d, timeout=%d)",self, self->fd, self->gpio.mode);
}



STATIC mp_obj_t mp_machine_gpio_value_get(machine_gpio_obj_t *self)
{
    if(ioctl(self->fd,GPIO_DM_INPUT,&(self->gpio)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin Input error"),self->gpio.pin);

    if(ioctl(self->fd,GPIO_READ_VALUE,&(self->gpio)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Read %d pin value error"),self->gpio.pin);
    return mp_obj_new_int(self->gpio.mode);
}

STATIC void mp_machine_gpio_value_set(machine_gpio_obj_t *self,mp_int_t value)
{
    // printf("value : %ld\n",value);
    if(ioctl(self->fd,GPIO_DM_OUTPUT,&(self->gpio)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin Output error"),self->gpio.pin);
    if(value == 0)
    {
        if(ioctl(self->fd,GPIO_WRITE_LOW,&(self->gpio)))
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin GPIO_WRITE_LOW error"),self->gpio.pin);
    }
    else
    {
        if(ioctl(self->fd,GPIO_WRITE_HIGH,&(self->gpio)))
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin GPIO_WRITE_HIGH error"),self->gpio.pin);
    }

}

STATIC mp_obj_t machine_gpio_obj_init_helper(machine_gpio_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT},
        { MP_QSTR_pull, MP_ARG_INT,{.u_int = -1}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    // pin_mode_t gpio_config;
    
    // configure mode
    mp_int_t pin_io_mode = args[ARG_mode].u_int;
    mp_int_t pin_to_pull = args[ARG_pull].u_int;
    self->gpio.mode = pin_io_mode;
    if (self->gpio.pin < MAX_GPIO_NUM) {
        if(pin_io_mode == GPIO_DM_OUTPUT && args[ARG_pull].u_int != -1){
            mp_raise_ValueError("When this pin is in output mode, it is not allowed to pull up and down.");
        }else{
            if(pin_io_mode == GPIO_OUTPUT)
            {
                if (args[ARG_value].u_int != -1) {
                    mp_machine_gpio_value_set(self, args[ARG_value].u_int);
                }
            }
            else{
                if(pin_to_pull == GPIO_INPUT)
                {
                    if(ioctl(self->fd,GPIO_DM_INPUT,&(self->gpio)))
                        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin mode error"),self->gpio.pin);
                }                    
                if(pin_to_pull == GPIO_INPUT_PULL_UP)
                {
                    if(ioctl(self->fd,GPIO_DM_INPUT_PULL_UP,&(self->gpio)))
                        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("GPIO_DM_INPUT_PULL_UP error"));
                }
                else if(pin_to_pull == GPIO_INPUT_PULL_DOWN)
                {
                    if(ioctl(self->fd,GPIO_DM_INPUT_PULL_DOWN,&(self->gpio)))
                        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("GPIO_DM_INPUT_PULL_DOWN error"));
                }
                else
                    mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("pin_to_pull error"));                     
            }
            
        }
    }
    else
        mp_raise_ValueError("pin not found");


    return mp_const_none;
}

STATIC mp_obj_t mp_machine_gpio_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 2, 4, true);

    if (gpio_fd < 0) {
        gpio_fd = open(device_name, O_RDWR);
        if (gpio_fd < 0)
            mp_raise_OSError_with_filename(errno, device_name);
    }
    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    machine_gpio_obj_t *self = mp_obj_malloc(machine_gpio_obj_t, &machine_gpio_type);
    if (0 <= wanted_pin && wanted_pin < MAX_GPIO_NUM) {
        self->gpio.pin = wanted_pin;
    }
    self->fd = gpio_fd;
    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_gpio_obj_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC  mp_obj_t machine_gpio_value(size_t n_args, const mp_obj_t *args) 
{
    machine_gpio_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    if (n_args == 1) {
        // Get value.
        return mp_machine_gpio_value_get(self);
    } else {
        // Set value.
        mp_int_t value = mp_obj_get_int(args[1]);
        mp_machine_gpio_value_set(self, value);
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_gpio_value_obj, 1,2,machine_gpio_value);

STATIC mp_obj_t machine_gpio_mode(mp_obj_t self_o, mp_obj_t mode) 
{
    machine_gpio_obj_t *self = MP_OBJ_TO_PTR(self_o);
    if(mp_obj_is_int(mode) == GPIO_OUTPUT)
    {
        if(ioctl(self->fd,GPIO_DM_OUTPUT,&(self->gpio)))
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin mode error"),self->gpio.pin);        
    }
    else
    {
        if(ioctl(self->fd,GPIO_DM_INPUT,&(self->gpio)))
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Set %d pin mode error"),self->gpio.pin);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_gpio_mode_obj, machine_gpio_mode);


STATIC mp_obj_t machine_gpio_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_wake ,ARG_priority};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_PE_BOTH} },
        { MP_QSTR_wake, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_INT, {.u_int = 7} },
    };
    machine_gpio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    signal(SIGUSR2,sigroutine);
    if ((n_args > 1 || kw_args->used != 0)) {
        // configure irq
        mp_obj_t handler = args[ARG_handler].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        mp_obj_t wake_obj = args[ARG_wake].u_obj;
        mp_int_t temp_wake_int;
        mp_obj_get_int_maybe(args[ARG_wake].u_obj,&temp_wake_int);
        if(wake_obj != mp_const_none && temp_wake_int != 0){
            mp_raise_ValueError("This platform does not support interrupt wakeup");
        }else{
            if (trigger == GPIO_PE_NONE || trigger == GPIO_PE_RISING || trigger == GPIO_PE_FALLING || trigger == GPIO_PE_BOTH) {
                if (handler == mp_const_none) {
                    handler = MP_OBJ_NULL;
                    trigger = 0;
                }
                self->callback = handler;
                // overall_self->callback = handler;
                if(ioctl(self->fd,KD_GPIO_PE_BOTH,&(self->gpio)))
                    mp_raise_ValueError("set GPIO trigger failed");
                
                if(ioctl(self->fd,KD_GPIO_ENABLE_IRQ,&(self->gpio)))
                    mp_raise_ValueError("set GPIO irq enable failed");                
            }else{

            }
        }

    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_gpio_irq_obj, 1, machine_gpio_irq);


STATIC mp_obj_t machine_gpio_disirq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    machine_gpio_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    if(ioctl(self->fd,KD_GPIO_DISABLE_IRQ,&(self->gpio)))
        mp_raise_ValueError("set GPIO trigger failed");   
    return mp_const_none;
}

STATIC  MP_DEFINE_CONST_FUN_OBJ_KW(machine_gpio_disirq_obj,1,machine_gpio_disirq);

STATIC const mp_rom_map_elem_t machine_gpio_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&machine_gpio_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_gpio_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_gpio_irq_obj) },
    { MP_ROM_QSTR(MP_QSTR_disirq), MP_ROM_PTR(&machine_gpio_disirq_obj) },
    { MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(GPIO_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(GPIO_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(GPIO_INPUT_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(GPIO_INPUT_PULL_DOWN) },
};
STATIC MP_DEFINE_CONST_DICT(machine_gpio_locals_dict, machine_gpio_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_gpio_type,
    MP_QSTR_GPIO,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_gpio_make_new,
    print, mp_machine_gpio_print,
    locals_dict, &machine_gpio_locals_dict
    );