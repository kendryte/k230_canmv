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

#pragma once

#ifndef _NNCASE_TYPE_H_
#define _NNCASE_TYPE_H_

#include "py/obj.h"
#include "nncase_wrap.h"


#define INTERP_METHOD_TF_NEAREST 0
#define INTERP_METHOD_TF_BILINEAR 1
#define INTERP_METHOD_CV2_NEAREST 2
#define INTERP_METHOD_CV2_BILINEAR 3


#define INTERP_MODE_NONE 0
#define INTERP_MODE_ALIGN_CORNER 1
#define INTERP_MODE_HALF_PIXEL 2

#define AI2D_FORMAT_YUV420_NV12 0
#define AI2D_FORMAT_YUV420_NV21 1
#define AI2D_FORMAT_YUV420_I420 2
#define AI2D_FORMAT_NCHW_FMT 3
#define AI2D_FORMAT_RGB_packed 4
#define AI2D_FORMAT_RAW16 5

struct finite_data{
    float data[8];
    size_t data_size;
};

struct infinite_data{
    uint8_t *data;
    size_t data_size;
};

struct ai2d_dtype_param {
    int src_format ;
    int dst_format  ;
    int src_type  ;
    int dst_type  ;
};
struct ai2d_crop_param
{
    bool flag ;
    int32_t start_x  ;
    int32_t start_y  ;
    int32_t width  ;
    int32_t height  ;
};
struct ai2d_shift_param {
    bool flag ;
    int32_t shift_value  ;
};
struct ai2d_pad_param {
    bool flag ;
    finite_data paddings;
    int pad_mode ;
    finite_data pad_value;
};
struct ai2d_resize_param {
    bool flag;
    int interp_method;
    int interp_mode;
};
struct ai2d_affine_param {
    bool flag ;
    int interp_method ;
    uint32_t cord_round ;
    uint32_t bound_ind ;
    int32_t bound_val ;
    uint32_t bound_smooth ;
    finite_data M;
};

struct tensor_desc
{
    int datatype;
    size_t start;
    size_t size;
};

struct rt_to_ndarray_info
{
    uint8_t dtype_;
    uint8_t ndim_;
    size_t len_;
    size_t shape_[8];
    size_t strides_[8];
    void *data_;
};

// 数据结构
// TODO: runtime_tensor
// TODO: 更改load model的输入数据，使用流，而不是直接使用字符串
typedef struct _kpu_obj_t
{
    mp_obj_base_t base;
    Kpu *interp;
} kpu_obj_t;

typedef struct _runtime_tensor_obj_t {
    mp_obj_base_t base;
    runtime_tensor *r_tensor;
} mp_runtime_tensor_obj_t;

typedef struct _tensor_desc_wrap_obj_t {
    mp_obj_base_t base;
    tensor_desc *t_desc;
} mp_tensor_desc_wrap_obj_t;


typedef struct _ai2d_obj_t {
    mp_obj_base_t base;
    ai2d *ai2d_;
} ai2d_obj_t;

typedef struct _builder_obj_t {
    mp_obj_base_t base;
    m_builder *builder;
} builder_obj_t;

// typedef struct _rt_to_ndarray_info_obj_t {
//     mp_obj_base_t base;
//     rt_to_ndarray_info *info;
// } mp_rt_to_ndarray_info_obj_t;


#endif // _NNCASE_TYPE_H_
