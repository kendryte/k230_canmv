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
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "sys/ioctl.h"
#include "py/runtime.h"
#include "py/obj.h"

#define SHA256_BLOCK_SIZE               32
#define HASH_DEVICE_NAME                "/dev/hwhash"
#define RT_HWHASH_CTRL_INIT             _IOWR('H', 0, int)
#define RT_HWHASH_CTRL_UPDATE           _IOWR('H', 1, int)
#define RT_HWHASH_CTRL_FINISH           _IOWR('H', 2, int)

typedef struct _mp_obj_hash_t {
    mp_obj_base_t base;
    int fd;
    bool final; // if set, update and digest raise an exception
    uintptr_t state[0]; // must be aligned to a machine word
} mp_obj_hash_t;

typedef struct {
    void *msg;
    void *dgst;
    uint32_t msglen;
    uint32_t dlen;
} hash_config_t;

STATIC hash_config_t hash_config = {
    .msg = NULL,
    .dgst = NULL,
    .msglen = 0,
    .dlen = 0,
};

static void hashlib_ensure_not_final(mp_obj_hash_t *self) {
    if (self->final) {
        mp_raise_ValueError(MP_ERROR_TEXT("hash is final"));
    }
}

STATIC mp_obj_t hashlib_sha256_update(mp_obj_t self_in, mp_obj_t arg);

STATIC mp_obj_t hashlib_sha256_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // hash_config_t config;
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    mp_obj_hash_t *o = mp_obj_malloc(mp_obj_hash_t, type);
    o->final = false;
    o->fd = -1;

    // open
    if(o->fd == -1)
    {
        o->fd = open(HASH_DEVICE_NAME, O_RDWR);
        if(o->fd < 0)
            mp_raise_OSError_with_filename(errno, HASH_DEVICE_NAME);
    }

    // init
    if(ioctl(o->fd, RT_HWHASH_CTRL_INIT, &hash_config))
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("sha256 initial config error!"));

    if(n_args == 1)
        hashlib_sha256_update(MP_OBJ_FROM_PTR(o), args[0]);

    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t hashlib_sha256_update(mp_obj_t self_in, mp_obj_t arg)
{
    mp_buffer_info_t bufinfo;
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    hashlib_ensure_not_final(self);
    mp_get_buffer_raise(arg, &bufinfo, MP_BUFFER_READ);
    // update
    hash_config.msg = bufinfo.buf;
    hash_config.msglen = bufinfo.len;
    if(ioctl(self->fd, RT_HWHASH_CTRL_UPDATE, &hash_config))
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("sha256 update config error!"));

    return mp_const_none;
}

STATIC mp_obj_t hashlib_sha256_digest(mp_obj_t self_in)
{
    mp_obj_hash_t *self = MP_OBJ_TO_PTR(self_in);
    hashlib_ensure_not_final(self);
    self->final = true;
    vstr_t vstr;
    vstr_init_len(&vstr, SHA256_BLOCK_SIZE);
    // final
    hash_config.dgst = (void *)vstr.buf;
    // hash_config.dlen = vstr.len;
    if(ioctl(self->fd, RT_HWHASH_CTRL_FINISH, &hash_config))
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("sha256 final config error!"));

    close(self->fd);
    return mp_obj_new_bytes_from_vstr(&vstr);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(hashlib_sha256_update_obj, hashlib_sha256_update);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(hashlib_sha256_digest_obj, hashlib_sha256_digest);

STATIC const mp_rom_map_elem_t hashlib_sha256_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&hashlib_sha256_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_digest), MP_ROM_PTR(&hashlib_sha256_digest_obj) },
};

STATIC MP_DEFINE_CONST_DICT(hashlib_sha256_locals_dict, hashlib_sha256_locals_dict_table);

STATIC MP_DEFINE_CONST_OBJ_TYPE(
    hashlib_sha256_type,
    MP_QSTR_sha256,
    MP_TYPE_FLAG_NONE,
    make_new, hashlib_sha256_make_new,
    locals_dict, &hashlib_sha256_locals_dict
    );

STATIC const mp_rom_map_elem_t mp_module_hashlib_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_hashlib) },
    { MP_ROM_QSTR(MP_QSTR_sha256), MP_ROM_PTR(&hashlib_sha256_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_hashlib_globals, mp_module_hashlib_globals_table);

const mp_obj_module_t mp_module_hashlib = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_hashlib_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_hashlib, mp_module_hashlib);