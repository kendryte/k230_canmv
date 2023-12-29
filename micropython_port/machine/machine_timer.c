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

typedef enum {
    MACHINE_TIMER_MODE_ONE_SHOT = 0,
    MACHINE_TIMER_MODE_PERIODIC,
    MACHINE_TIMER_MODE_MAX
} machine_timer_mode_t;

typedef struct _machine_timer_obj_t {
    mp_obj_base_t          base;
    machine_timer_mode_t   mode;
    uint64_t               period;
    mp_obj_t               callback;
    mp_obj_t               arg;
    timer_t                timerid;
    bool                   active;
    bool                   valid;
} machine_timer_obj_t;

STATIC void _machine_timer_start(machine_timer_obj_t *self);

static machine_timer_obj_t *timer_obj;

STATIC void timer_handler(int sig, siginfo_t *si, void *uc) {
    // TODO: wait rtt-smart support sigaction
    // machine_timer_obj_t *self = (machine_timer_obj_t *)si->si_value.sival_ptr;
    machine_timer_obj_t *self = timer_obj;
    if (self == NULL)
        return;
    if (mp_sched_schedule(self->callback, self->arg) == 0)
        printf("[Timer] schedule queue is full\n");
    if (self->mode == MACHINE_TIMER_MODE_ONE_SHOT)
        self->active = false;
}

STATIC void machine_timer_obj_check(machine_timer_obj_t *self) {
    if (self->valid == 0)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer object has been deleted"));
}

STATIC void mp_machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;

    machine_timer_obj_check(self);

    mp_printf(print, "Timer: id=%u, period=%uus, mode=%s, callback=%p, arg=%p, status:%s\n",
        self->timerid, self->period, self->mode == MACHINE_TIMER_MODE_ONE_SHOT ? "oneshot" : "periodic",
        self->callback, self->arg, self->active ? "runing" : "stoped");
}

STATIC void machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum {
        ARG_period,
        ARG_callback,
        ARG_arg,
        ARG_mode,
        ARG_start,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_period,        MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_callback,      MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_arg,           MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_mode,          MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MACHINE_TIMER_MODE_ONE_SHOT} },
        { MP_QSTR_start,         MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!mp_obj_is_callable(args[ARG_callback].u_obj))
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("callback is not a function"));

    self->mode = args[ARG_mode].u_int;
    self->period = args[ARG_period].u_int;
    self->callback = args[ARG_callback].u_obj;
    self->arg = args[ARG_arg].u_obj;

    if (args[ARG_start].u_bool)
        _machine_timer_start(self);
}

STATIC mp_obj_t mp_machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    static bool is_init = false;
    timer_t timerid;
    struct sigevent sev;

    mp_arg_check_num(n_args, n_kw, 3, 3, true);
    if (timer_obj != NULL)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer object has been created"));
    machine_timer_obj_t *self = m_new_obj_with_finaliser(machine_timer_obj_t);
    self->base.type = &machine_timer_type;

    if (!is_init) {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = timer_handler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM, &sa, NULL);
        is_init = true;
    }

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = self;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer create error"));
    self->timerid = timerid;
    self->valid = true;

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_timer_init_helper(self, n_args, args, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void _machine_timer_start(machine_timer_obj_t *self) {
    machine_timer_obj_t *tmp;
    struct itimerspec its = {};

    its.it_value.tv_sec = self->period / 1000000;
    its.it_value.tv_nsec = self->period % 1000000 * 1000;
    if (self->mode == MACHINE_TIMER_MODE_PERIODIC) {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }

    self->active = true;
    tmp = timer_obj;
    timer_obj = self;
    if (timer_settime(self->timerid, 0, &its, NULL)) {
        timer_obj = tmp;
        self->active = false;
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer set error"));
    }
}

STATIC mp_obj_t machine_timer_start(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;

    machine_timer_obj_check(self);
    if (self->active)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer is running"));

    _machine_timer_start(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_start_obj, machine_timer_start);

STATIC mp_obj_t machine_timer_restart(mp_obj_t self_in) {
    machine_timer_obj_t *self = self_in;

    machine_timer_obj_check(self);
    _machine_timer_start(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_restart_obj, machine_timer_restart);

STATIC mp_obj_t machine_timer_del(mp_obj_t self_in) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->valid)
        return mp_const_none;

    if (timer_delete(self->timerid))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("del timer error"));

    self->valid = false;
    timer_obj = NULL;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_del_obj, machine_timer_del);

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    machine_timer_obj_check(self);
    machine_timer_del(self_in);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    machine_timer_obj_check(self);
    if (self->active)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer is running"));
    machine_timer_init_helper(self, n_args - 1, args + 1, kw_args);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 0, machine_timer_init);

STATIC mp_obj_t machine_timer_stop(mp_obj_t self_in) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    struct itimerspec its;

    machine_timer_obj_check(self);

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 0;
    if (timer_settime(self->timerid, 0, &its, NULL))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer set error"));
    self->active = false;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_stop_obj, machine_timer_stop);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_del_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&machine_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&machine_timer_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart), MP_ROM_PTR(&machine_timer_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_MODE_ONE_SHOT), MP_ROM_INT(MACHINE_TIMER_MODE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_MODE_PERIODIC), MP_ROM_INT(MACHINE_TIMER_MODE_PERIODIC) },
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
