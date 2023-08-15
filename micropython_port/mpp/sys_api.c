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
#include <unistd.h>
#include <errno.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "mpi_sys_api.h"

#define DEF_FUNC_IMPL_MMZ_ALLOC(x)                                      \
STATIC mp_obj_t _##x(size_t n_args, const mp_obj_t *args) {             \
    mp_buffer_info_t bufinfo[2];                                        \
    mp_get_buffer_raise(args[0], &bufinfo[0], MP_BUFFER_READ);          \
    mp_get_buffer_raise(args[1], &bufinfo[1], MP_BUFFER_READ);          \
    size_t ret = x((k_u64 *)(bufinfo[0].buf), (void **)(bufinfo[1].buf),\
        mp_obj_str_get_str(args[2]), mp_obj_str_get_str(args[3]),       \
        mp_obj_get_int(args[4]));                                       \
    return mp_obj_new_int(ret);                                         \
}                                                                       \
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(x##_obj, 5, 5, _##x);

#define DEF_FUNC_ADD(x) { MP_ROM_QSTR(MP_QSTR_##x), MP_ROM_PTR(&x##_obj) },

DEF_FUNC_IMPL_MMZ_ALLOC(kd_mpi_sys_mmz_alloc)
DEF_FUNC_IMPL_MMZ_ALLOC(kd_mpi_sys_mmz_alloc_cached)

#define FUNC_IMPL
#define FUNC_FILE "sys_func_def.h"
#include "func_def.h"

STATIC const mp_rom_map_elem_t sys_api_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_sys_api) },
    DEF_FUNC_ADD(kd_mpi_sys_mmz_alloc)
    DEF_FUNC_ADD(kd_mpi_sys_mmz_alloc_cached)
#define FUNC_ADD
#define FUNC_FILE "sys_func_def.h"
#include "func_def.h"
};
STATIC MP_DEFINE_CONST_DICT(sys_api_locals_dict, sys_api_locals_dict_table);

const mp_obj_module_t mp_module_sys_api = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&sys_api_locals_dict,
};
