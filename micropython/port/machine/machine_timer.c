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

#define TIMER_NUMBER_MAX 6

enum {
    MODE_ONE_SHOT = 0,
    MODE_PERIODIC,
};

typedef struct {
    mp_obj_base_t base;
    int8_t index;
    uint8_t mode;
    uint8_t status;
    timer_t timerid;
    uint64_t period;
    mp_obj_t callback;
    mp_obj_t arg;
} machine_timer_obj_t;

static bool timer_used[TIMER_NUMBER_MAX] = {1, 1, 1, 1, 1, 1};
static machine_timer_obj_t *timer_obj;

STATIC void timer_handler(int sig, siginfo_t *si, void *uc) {
    // TODO: wait rtt-smart support sigaction
    // machine_timer_obj_t *self = (machine_timer_obj_t *)si->si_value.sival_ptr;
    machine_timer_obj_t *self = timer_obj;
    if (self == NULL) {
        return;
    }
    if (mp_sched_schedule(self->callback, self->arg) == 0) {
        printf("[Timer] schedule queue is full\n");
    }
}

STATIC void machine_timer_obj_check(machine_timer_obj_t *self) {
    if (self->status == 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer object has been deleted"));
    }
}

STATIC void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_timer_obj_t *self = self_in;
    machine_timer_obj_check(self);

    mp_printf(print, "Timer %d: id=%u, period=%uus, mode=%s, callback=%p, arg=%p\n",
        self->index, self->timerid, self->period, self->mode == MODE_ONE_SHOT ? "oneshot" : "periodic",
        self->callback, self->arg);
}

STATIC void _machine_timer_start(machine_timer_obj_t *self) {
    if (self->index == -1) {
        struct itimerspec its = {};

        its.it_value.tv_sec = self->period / 1000000;
        its.it_value.tv_nsec = self->period % 1000000 * 1000;
        if (self->mode == MODE_PERIODIC) {
            its.it_interval.tv_sec = its.it_value.tv_sec;
            its.it_interval.tv_nsec = its.it_value.tv_nsec;
        }

        timer_obj = self;
        if (timer_settime(self->timerid, 0, &its, NULL)) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The soft timer set error"));
        }
    }
}

STATIC void machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_freq, ARG_period, ARG_callback, ARG_arg };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = MODE_PERIODIC } },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_period, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_callback, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_arg, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (!mp_obj_is_callable(args[ARG_callback].u_obj)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("callback is not a function"));
    }
    float freq = -1;
    int period = -1;
    if (args[ARG_freq].u_obj != mp_const_none) {
        freq = mp_obj_get_float(args[ARG_freq].u_obj);
        if (freq > 0) {
            period = 1e6 / freq;
        }
    }
    if (period == -1) {
        period = args[ARG_period].u_int * 1000;
    }
    if (period <= 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("please set valid freq or period"));
    }

    self->period = period;
    self->mode = args[ARG_mode].u_int;
    self->callback = args[ARG_callback].u_obj;
    self->arg = args[ARG_arg].u_obj;

    _machine_timer_start(self);
}

STATIC mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 6, true);

    int index = mp_obj_get_int(args[0]);
    if (index == -1) {
        if (timer_obj != NULL) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer object has been created"));
        }
    } else if (index < 0 || index >= TIMER_NUMBER_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid timer number"));
    } else {
        if (timer_used[index]) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Timer %u busy"), index);
        }
    }

    static bool is_init = false;
    if (!is_init) {
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = timer_handler;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGALRM, &sa, NULL);
        is_init = true;
    }

    machine_timer_obj_t *self = m_new_obj_with_finaliser(machine_timer_obj_t);
    self->base.type = &machine_timer_type;

    if (index == -1) { // soft timer
        timer_t timerid;
        struct sigevent sev;
        sev.sigev_notify = SIGEV_SIGNAL;
        sev.sigev_signo = SIGALRM;
        sev.sigev_value.sival_ptr = self;
        if (timer_create(CLOCK_REALTIME, &sev, &timerid)) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("The timer create error"));
        }
        self->timerid = timerid;
    } else { // hard timer
        char dev_name[16] = "/dev/hwtimer0";
        dev_name[12] = '0' + index;
        int fd = open(dev_name, O_RDWR);
        if (fd < 0) {
            mp_raise_OSError_with_filename(errno, dev_name);
        }
        self->timerid = fd;
    }

    self->index = index;
    self->status = 1;
    self->mode = 0;
    self->period = 0;
    self->callback = 0;
    self->arg = 0;

    if (n_kw > 1) {
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_timer_init_helper(self, n_args, args, &kw_args);
    }
    self->status = 2;
    if (index == -1) {
        timer_obj = self;
    } else {
        timer_used[self->index] = 1;
    }

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->status == 0)
        return mp_const_none;

    if (self->index == -1) {
        if (timer_delete(self->timerid)) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("del timer error"));
        }
        timer_obj = NULL;
    } else {
        close(self->timerid);
        if (self->status == 2) {
            timer_used[self->index] = 0;
        }
    }

    self->status = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);

STATIC mp_obj_t machine_timer_init(mp_uint_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    machine_timer_obj_check(self);
    machine_timer_init_helper(self, n_args - 1, args + 1, kw_args);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 0, machine_timer_init);

STATIC const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(MODE_ONE_SHOT) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(MODE_PERIODIC) },
};
STATIC MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, machine_timer_make_new,
    print, machine_timer_print,
    locals_dict, &machine_timer_locals_dict
    );
