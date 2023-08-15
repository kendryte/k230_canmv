/*
 * This file is part of the OpenMV project.
 *
 * Copyright (c) 2013-2021 Ibrahim Abdelkader <iabdalkader@openmv.io>
 * Copyright (c) 2013-2021 Kwabena W. Agyeman <kwagyeman@openmv.io>
 *
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * Mutex implementation.
 * This is a standard implementation of mutexs on ARM processors following the ARM guide.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0321a/BIHEJCHB.html
 *
 * Note: The Cortex-M0/M0+ does Not have the Load/Store exclusive instructions, on these
 * CPUs the locking function is implemented with atomic access using disable/enable IRQs.
 */
#include "mutex.h"
#include "py/mphal.h"

void mutex_init0(omv_mutex_t *mutex) {
    mutex->tid = 0;
    mutex->lock = 0;
    mutex->last_tid = 0;
}

static void _mutex_lock(omv_mutex_t *mutex, uint32_t tid, bool blocking) {
    
}

void mutex_lock(omv_mutex_t *mutex, uint32_t tid) {
    _mutex_lock(mutex, tid, true);
}

int mutex_try_lock(omv_mutex_t *mutex, uint32_t tid) {
    // If the mutex is already locked by the current thread then
    // release it and return without locking, otherwise try to lock it.
    if (mutex->tid == tid) {
        mutex_unlock(mutex, tid);
    } else {
        _mutex_lock(mutex, tid, false);
    }

    return (mutex->tid == tid);
}

int mutex_try_lock_alternate(omv_mutex_t *mutex, uint32_t tid) {
    if (mutex->last_tid != tid) {
        if (mutex_try_lock(mutex, tid)) {
            mutex->last_tid = tid;
            return 1;
        }
    }

    return 0;
}

int mutex_lock_timeout(omv_mutex_t *mutex, uint32_t tid, uint32_t timeout) {

    return 0;
}

void mutex_unlock(omv_mutex_t *mutex, uint32_t tid) {
    if (mutex->tid == tid) {
        mutex->tid = 0;
        mutex->lock = 0;
    }
}
