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
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include "sys/ioctl.h"
#include "machine_timer.h"
#include "py/runtime.h"
#include "py/obj.h"

typedef enum _timer_channel_number
{
    TIMER_CHANNEL_0,
    TIMER_CHANNEL_1,
    TIMER_CHANNEL_2,
    TIMER_CHANNEL_3,
    TIMER_CHANNEL_4,
    TIMER_CHANNEL_5,
    TIMER_CHANNEL_MAX,
} timer_channel_number_t;

typedef enum{
    MACHINE_TIMER_MODE_ONE_SHOT = 0,
    MACHINE_TIMER_MODE_PERIODIC,
    MACHINE_TIMER_MODE_PWM,
    MACHINE_TIMER_MODE_MAX
}machine_timer_mode_t;

typedef enum{
    MACHINE_TIMER_UNIT_NS = 0 ,
    MACHINE_TIMER_UNIT_US ,
    MACHINE_TIMER_UNIT_MS ,
    MACHINE_TIMER_UNIT_S  ,
    MACHINE_TIMER_UNIT_MAX
}machine_timer_unit_t;

typedef struct _machine_timer_obj_t {
    mp_obj_base_t          base;
    machine_timer_mode_t   mode;
	mp_uint_t              period;    //(0,2^32)
    machine_timer_unit_t   unit;      //machine_timer_unit_t
	mp_obj_t               callback;
    mp_obj_t               arg;
    uint32_t               priority;  //[1,7]
    bool                   active;
    timer_t                soft_timer;
} machine_timer_obj_t;

const mp_obj_type_t machine_timer_type;

STATIC bool check_unit(uint32_t unit)
{
    if( unit>=MACHINE_TIMER_UNIT_MAX )
        return false;
    return true;
}

machine_timer_obj_t *self;


STATIC void sigroutine(int signo)
{
   switch (signo){
   case SIGALRM:
        mp_sched_schedule(self->callback,self->arg);
        break;
   case SIGUSR1:
        printf("Catch a signal -- SIGUSR1 \n");
        break;
   }
}

machine_timer_obj_t *timer_callback;

STATIC void mp_machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;
    char* unit;
    if (self->unit == MACHINE_TIMER_UNIT_MS)
        unit = "ms";
    else if(self->unit == MACHINE_TIMER_UNIT_S)
        unit = "s";
    else if(self->unit == MACHINE_TIMER_UNIT_US)
        unit = "us";
    else
        unit = "ns";
    mp_printf(print, 
        "[CANMV]Timer:(%p) period=%d%s, priority=%d,  callback=%p, arg=%p\r\n",
        self,self->period, unit, self->priority, self->callback, self->arg);
}


STATIC mp_obj_t machine_timer_start(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;
    struct itimerspec s_value;
    if(self->unit == MACHINE_TIMER_UNIT_NS)
    {

        s_value.it_interval.tv_sec = 0;
        if(self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        {
            s_value.it_interval.tv_nsec = 0;
        }
        else{
            s_value.it_interval.tv_nsec = self->period;
        }
        s_value.it_value.tv_sec = 0;
        s_value.it_value.tv_nsec = self->period;
    }
    else if(self->unit == MACHINE_TIMER_UNIT_US)
    {

        s_value.it_interval.tv_sec = 0;
        if(self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        {
            s_value.it_interval.tv_nsec = 0;
        }
        else{
            s_value.it_interval.tv_nsec = self->period * 1000;
        }
        s_value.it_value.tv_sec = 0;
        s_value.it_value.tv_nsec = self->period * 1000;
    }
    else if(self->unit == MACHINE_TIMER_UNIT_MS)
    {
        s_value.it_interval.tv_sec = 0;

        if(self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        {
            s_value.it_interval.tv_nsec = 0;
        }
        else
        {
            s_value.it_interval.tv_nsec = self->period * 1000000;
        }
        s_value.it_value.tv_sec = 0;
        s_value.it_value.tv_nsec = self->period * 1000000;
    }
    else if(self->unit == MACHINE_TIMER_UNIT_S)
    {
        if(self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        {
            s_value.it_interval.tv_sec = 0;
        }
        else
        {
            s_value.it_interval.tv_sec = self->period;    
        }
        s_value.it_interval.tv_nsec = 0;
        s_value.it_value.tv_sec = self->period;
        s_value.it_value.tv_nsec = 0;
    }
    if(self->active == false)
    {
        timer_settime(self->soft_timer,0,&s_value,NULL);
    }

	self->active = true;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_start_obj, machine_timer_start);

STATIC mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    bool is_start;
    struct sigevent evp;
    enum {
        ARG_mode,
        ARG_period,
        ARG_unit,
        ARG_callback,
        ARG_arg,
        ARG_start,
        ARG_priority,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_TIMER_MODE_ONE_SHOT} },
        { MP_QSTR_period,        MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },  //默认1s             
        { MP_QSTR_unit,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_TIMER_UNIT_MS} },
        { MP_QSTR_callback,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_arg,         MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_start,         MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = true} },
        { MP_QSTR_priority,      MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    self->mode = args[ARG_mode].u_int;
    self->period = args[ARG_period].u_int;
    self->unit = args[ARG_unit].u_int;
    self->callback = args[ARG_callback].u_obj;
    self->arg = args[ARG_arg].u_obj;
    is_start = args[ARG_start].u_bool;
    self->priority = args[ARG_priority].u_int;
    memset(&evp, 0, sizeof(struct sigevent));
    // pthread_t callback_handle;
    // pthread_create(&callback_handle, NULL, thread_callback,(void*)self);
    evp.sigev_value.sival_ptr = &(self->soft_timer);
    evp.sigev_signo = SIGALRM;
    evp.sigev_notify = SIGEV_SIGNAL;
    signal(evp.sigev_signo,sigroutine);

    if(timer_create(CLOCK_REALTIME,&evp,&(self->soft_timer)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("timer_create error"));
    if(is_start == true)
    {
        machine_timer_start(self);
    }
    return mp_const_none;
}

STATIC mp_obj_t mp_machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 7, true);
    self = m_new_obj(machine_timer_obj_t);
    self->base.type = &machine_timer_type;

	mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
	machine_timer_init_helper(self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(timer_delete(self->soft_timer))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("deinit timer error"));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    // machine_timer_obj_t *self = m_new_obj(machine_timer_obj_t);
    // self->base.type = &machine_timer_type;
	return machine_timer_init_helper(args[0], n_args - 1, args + 1,kw_args);

    // return MP_OBJ_FROM_PTR(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 0, machine_timer_init);


STATIC mp_obj_t machine_timer_callbcak(mp_obj_t self_in,mp_obj_t func_in) {
    machine_timer_obj_t *self = self_in;
	
    self->callback = func_in;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_timer_callback_obj, machine_timer_callbcak);

STATIC mp_obj_t machine_timer_period(size_t n_args, const mp_obj_t *args) {
    machine_timer_obj_t *self = args[0];
    if( n_args == 1)//get period
    {
        return mp_obj_new_int(self->period);
    }
    if( n_args == 3)
    {
        if(!check_unit(mp_obj_get_int(args[2])))
            mp_raise_ValueError("[CANMV]Timer:unit error");
        self->unit = mp_obj_get_int(args[2]);
    }
    self->period = mp_obj_get_int(args[1]);

    return mp_obj_new_int(self->period);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_timer_period_obj, 1, 3, machine_timer_period);

STATIC mp_obj_t machine_timer_stop(mp_obj_t self_in) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    struct itimerspec d_value;
    d_value.it_interval.tv_sec = 0;
    d_value.it_interval.tv_nsec = 0;
    d_value.it_value.tv_sec = 0;
    d_value.it_value.tv_nsec = 0;
    if(timer_settime(self->soft_timer,0,&d_value,NULL))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("machine_timer_stop error"));
    return mp_const_none;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_stop_obj, machine_timer_stop);



STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_callback), MP_ROM_PTR(&machine_timer_callback_obj) },
    { MP_ROM_QSTR(MP_QSTR_period), MP_ROM_PTR(&machine_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_stop_obj) },
    // { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_timer_restart_obj) },
    // { MP_ROM_QSTR(MP_QSTR_callback_arg), MP_ROM_PTR(&machine_timer_callback_arg_obj) },
    // { MP_ROM_QSTR(MP_QSTR_time_left), MP_ROM_PTR(&machine_timer_time_left_obj) },
    // { MP_ROM_QSTR(MP_QSTR_freq), MP_ROM_PTR(&machine_timer_freq_obj) },
    // { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ONE_SHOT), MP_ROM_INT(MACHINE_TIMER_MODE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_MODE_PERIODIC), MP_ROM_INT(MACHINE_TIMER_MODE_PERIODIC) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_S),  MP_ROM_INT(MACHINE_TIMER_UNIT_S)  },
    { MP_ROM_QSTR(MP_QSTR_UNIT_MS), MP_ROM_INT(MACHINE_TIMER_UNIT_MS) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_US), MP_ROM_INT(MACHINE_TIMER_UNIT_US) },
    { MP_ROM_QSTR(MP_QSTR_UNIT_NS), MP_ROM_INT(MACHINE_TIMER_UNIT_NS) },

};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, mp_machine_timer_make_new,
    print, mp_machine_timer_print,
    locals_dict, &machine_timer_locals_dict
    );