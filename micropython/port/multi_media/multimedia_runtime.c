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
#include "multimedia_type.h"
#include "multimedia_wrap.h"
#include "RtspServer.h"

#define MICROPY_PY_MULTIMEDIA_RUNTIME      (1)

#if MICROPY_PY_MULTIMEDIA_RUNTIME

STATIC const mp_rom_map_elem_t multimedia_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_multimedia) },
    { MP_ROM_QSTR(MP_QSTR_rtsp_server), MP_ROM_PTR(&rtsp_server_type) },
    { MP_ROM_QSTR(MP_QSTR_multi_media_type), MP_ROM_PTR(&multi_media_type) },
};

STATIC MP_DEFINE_CONST_DICT(multimedia_module_globals, multimedia_module_globals_table);

const mp_obj_module_t mp_module_multimedia = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&multimedia_module_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_multimedia, mp_module_multimedia);

#endif // MICROPY_PY_MULTIMEDIA_RUNTIME