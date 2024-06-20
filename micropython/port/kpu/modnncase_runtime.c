/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Paul Sokolovsky
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

#include <stdio.h>
#include <stdint.h>

#include "py/runtime.h"
#include "py/binary.h"
#include "py/obj.h"
#include "nncase_type.h"
#include "nncase_wrap.h"
#include "kpu.h"
#include "ai2d.h"
#include "ndarray.h"
#include "ulab.h"

#include <string.h>

#if MICROPY_PY_NNCASE_RUNTIME


STATIC mp_obj_t mp_from_numpy(mp_obj_t ndarray)
{
    ndarray_obj_t *self = MP_ROM_PTR(ndarray);
    mp_runtime_tensor_obj_t *tensor = m_new_obj_with_finaliser(mp_runtime_tensor_obj_t);
    finite_data shape;
    shape.data_size = self->ndim;
    int all_size = 1;
    for (int i = 0; i < self->ndim; i++)
    {
        shape.data[i] = (float)self->shape[ULAB_MAX_DIMS - self->ndim + i];
        all_size*=self->shape[ULAB_MAX_DIMS - self->ndim + i];
    }
    
    int dtype = mp_dtype_to_nncase((char)self->dtype);
    tensor->r_tensor = from_numpy(dtype, shape, self->array, self->phy_addr);
    tensor->base.type = &rt_type;
    return MP_OBJ_TO_PTR(tensor);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_from_numpy_obj, mp_from_numpy);

STATIC mp_obj_t mp_shrink_memory_pool()
{
    shrink_memory_pool();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_shrink_memory_pool_obj, mp_shrink_memory_pool);

STATIC mp_obj_t mp_version()
{
    char* v = version();
    mp_obj_t mp_v_string;
    mp_v_string = mp_obj_new_str(v, strlen(v));
    return MP_OBJ_TO_PTR(mp_v_string);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_version_obj, mp_version);

STATIC const mp_rom_map_elem_t nncase_runtime_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_nncase_runtime) },
    { MP_ROM_QSTR(MP_QSTR_kpu), MP_ROM_PTR(&kpu_type) },
    { MP_ROM_QSTR(MP_QSTR_ai2d), MP_ROM_PTR(&ai2d_type) },
    { MP_ROM_QSTR(MP_QSTR_interp_method), MP_ROM_PTR(&interp_method_type) },
    { MP_ROM_QSTR(MP_QSTR_interp_mode), MP_ROM_PTR(&interp_mode_type) },
    { MP_ROM_QSTR(MP_QSTR_ai2d_format), MP_ROM_PTR(&ai2d_format_type) },
    { MP_ROM_QSTR(MP_QSTR_from_numpy), MP_ROM_PTR(&mp_from_numpy_obj) },
    { MP_ROM_QSTR(MP_QSTR_shrink_memory_pool), MP_ROM_PTR(&mp_shrink_memory_pool_obj) },
    { MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_version_obj) },
};

STATIC MP_DEFINE_CONST_DICT(nncase_runtime_module_globals, nncase_runtime_module_globals_table);

const mp_obj_module_t mp_module_nncase_runtime = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&nncase_runtime_module_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_nncase_runtime, mp_module_nncase_runtime);

#endif // 
 