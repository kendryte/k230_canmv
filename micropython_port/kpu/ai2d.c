// #include "nncase_wrap.h"
#include "nncase_wrap.h"
#include "nncase_type.h"
#include "ai2d.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

// utils
typedef struct {
    uint8_t size;
    char type;
    uint8_t data[32];
} my_struct;

static void obj_to_struct(mp_obj_t obj, my_struct *s)
{
    char typecode = s->type;
    s->size = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(obj));
    if (s->size > 8)
        mp_raise_msg_varg(&mp_type_TypeError,
            MP_ERROR_TEXT("obj len exceed 8"));
    mp_obj_t iter = mp_getiter(obj, NULL);
    mp_obj_t next = MP_OBJ_NULL;
    for (int i = 0; i < s->size; i++) {
        next = mp_iternext(iter);
        mp_binary_set_val_array(typecode, (void *)s->data, i, next);
    }
}


void _kd_mpi_struct_test_f(mp_obj_t obj, float *result, size_t *data_size) {
    my_struct s;
    s.type = 'f';
    obj_to_struct(obj, &s);
    float *data = (float *)s.data;
    for (int i = 0; i < s.size; i++)
    {
        result[i] = data[i];
    }
    *data_size = s.size;
}

void _kd_mpi_struct_test_I(mp_obj_t obj, float *result, size_t *data_size) {
    my_struct s;
    s.type = 'I';
    obj_to_struct(obj, &s);
    uint32_t *data = (uint32_t *)s.data;
    for (int i = 0; i < s.size; i++)
    {
        result[i] = (float)data[i];
    }
    *data_size = s.size;
}



// TODO:
// interp_method
STATIC const mp_rom_map_elem_t mp_interp_method_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tf_nearest), MP_ROM_INT(INTERP_METHOD_TF_NEAREST) },
    { MP_ROM_QSTR(MP_QSTR_tf_bilinear), MP_ROM_INT(INTERP_METHOD_TF_BILINEAR) },
    { MP_ROM_QSTR(MP_QSTR_cv2_nearest), MP_ROM_INT(INTERP_METHOD_CV2_NEAREST) },
    { MP_ROM_QSTR(MP_QSTR_cv2_bilinear), MP_ROM_INT(INTERP_METHOD_CV2_BILINEAR) },
};

STATIC MP_DEFINE_CONST_DICT(mp_interp_method_locals_dict, mp_interp_method_locals_dict_table);

const mp_obj_module_t mp_module_interp_method = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_interp_method_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_interp_method, mp_module_interp_method);

MP_DEFINE_CONST_OBJ_TYPE(
    interp_method_type,
    MP_QSTR_interp_method,
    MP_TYPE_FLAG_INSTANCE_TYPE,
    locals_dict, &mp_interp_method_locals_dict
    );

// interp_mode
STATIC const mp_rom_map_elem_t mp_interp_mode_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_none), MP_ROM_INT(INTERP_MODE_NONE) },
    { MP_ROM_QSTR(MP_QSTR_align_corner), MP_ROM_INT(INTERP_MODE_ALIGN_CORNER) },
    { MP_ROM_QSTR(MP_QSTR_half_pixel), MP_ROM_INT(INTERP_MODE_HALF_PIXEL) },
};
STATIC MP_DEFINE_CONST_DICT(mp_interp_mode_locals_dict, mp_interp_mode_locals_dict_table);

const mp_obj_module_t mp_module_interp_mode = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_interp_mode_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_interp_mode, mp_module_interp_mode);

MP_DEFINE_CONST_OBJ_TYPE(
    interp_mode_type,
    MP_QSTR_interp_mode,
    MP_TYPE_FLAG_INSTANCE_TYPE,
    locals_dict, &mp_interp_mode_locals_dict
    );
    

// interp_mode
STATIC const mp_rom_map_elem_t mp_ai2d_format_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_YUV420_NV12), MP_ROM_INT(AI2D_FORMAT_YUV420_NV12) },
    { MP_ROM_QSTR(MP_QSTR_YUV420_NV21), MP_ROM_INT(AI2D_FORMAT_YUV420_NV21) },
    { MP_ROM_QSTR(MP_QSTR_YUV420_I420), MP_ROM_INT(AI2D_FORMAT_YUV420_I420) },
    { MP_ROM_QSTR(MP_QSTR_NCHW_FMT), MP_ROM_INT(AI2D_FORMAT_NCHW_FMT) },
    { MP_ROM_QSTR(MP_QSTR_RGB_packed), MP_ROM_INT(AI2D_FORMAT_RGB_packed) },
    { MP_ROM_QSTR(MP_QSTR_RAW16), MP_ROM_INT(AI2D_FORMAT_RAW16) },
};
STATIC MP_DEFINE_CONST_DICT(mp_ai2d_format_locals_dict, mp_ai2d_format_locals_dict_table);

const mp_obj_module_t mp_module_ai2d_format = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_ai2d_format_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_ai2d_format, mp_module_ai2d_format);

MP_DEFINE_CONST_OBJ_TYPE(
    ai2d_format_type,
    MP_QSTR_ai2d_format,
    MP_TYPE_FLAG_INSTANCE_TYPE,
    locals_dict, &mp_ai2d_format_locals_dict
    );


// 实现各个功能函数
// init
STATIC mp_obj_t mp_ai2d_create() {
    ai2d_obj_t *self = m_new_obj_with_finaliser(ai2d_obj_t);
    self->ai2d_ = ai2d_create();
    self->base.type = &ai2d_type;
    return MP_OBJ_FROM_PTR(self);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mp_ai2d_create_obj, mp_ai2d_create);

// release
STATIC mp_obj_t mp_ai2d_destroy(mp_obj_t self_in) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(self_in);
    ai2d_destroy(self->ai2d_);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_ai2d_destroy_obj, mp_ai2d_destroy);


// set build
STATIC mp_obj_t mp_ai2d_build(mp_obj_t self_in, mp_obj_t in_shape, mp_obj_t out_shape) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(self_in);
    finite_data input_shape;
    _kd_mpi_struct_test_I(in_shape, input_shape.data, &input_shape.data_size);
    
    finite_data output_shape;
    _kd_mpi_struct_test_I(out_shape, output_shape.data, &output_shape.data_size);

    builder_obj_t* ai2d_builder_ = m_new_obj_with_finaliser(builder_obj_t);

    ai2d_builder_->builder = ai2d_build(self->ai2d_, input_shape, output_shape);

    ai2d_builder_->base.type = &ai2d_builder_type;
    return ai2d_builder_;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_ai2d_build_obj, mp_ai2d_build);




STATIC mp_obj_t mp_ai2d_set_dtype(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_dtype_param dt;
    dt.src_format = mp_obj_get_int(args[1]);
    dt.dst_format = mp_obj_get_int(args[2]);
    dt.src_type = mp_dtype_to_nncase(mp_obj_get_int(args[3]));
    dt.dst_type = mp_dtype_to_nncase(mp_obj_get_int(args[4]));
    ai2d_set_dtype(self->ai2d_, dt);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_dtype_obj, 5, 5, mp_ai2d_set_dtype);

STATIC mp_obj_t mp_ai2d_set_crop_param(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_crop_param cp;
    cp.flag = mp_obj_is_true(args[1]);
    cp.start_x = mp_obj_get_int(args[2]);
    cp.start_y = mp_obj_get_int(args[3]);
    cp.width = mp_obj_get_int(args[4]);
    cp.height = mp_obj_get_int(args[5]);
    ai2d_set_crop_param(self->ai2d_, cp);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_crop_param_obj, 6, 6, mp_ai2d_set_crop_param);

STATIC mp_obj_t mp_ai2d_set_shift_param(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_shift_param sp;
    sp.flag = mp_obj_is_true(args[1]);
    sp.shift_value = mp_obj_get_int(args[2]);
    ai2d_set_shift_param(self->ai2d_, sp);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_shift_param_obj, 3, 3, mp_ai2d_set_shift_param);

STATIC mp_obj_t mp_ai2d_set_pad_param(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_pad_param pp;
    pp.flag = mp_obj_is_true(args[1]);
    _kd_mpi_struct_test_I(args[2], pp.paddings.data, &pp.paddings.data_size);
    pp.pad_mode = mp_obj_get_int(args[3]);
    _kd_mpi_struct_test_I(args[4], pp.pad_value.data, &pp.pad_value.data_size);
    ai2d_set_pad_param(self->ai2d_, pp);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_pad_param_obj, 5, 5, mp_ai2d_set_pad_param);


STATIC mp_obj_t mp_ai2d_set_resize_param(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_resize_param rp;
    rp.flag = mp_obj_is_true(args[1]);
    rp.interp_method = mp_obj_get_int(args[2]);
    rp.interp_mode = mp_obj_get_int(args[3]);
    ai2d_set_resize_param(self->ai2d_, rp);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_resize_param_obj, 4, 4, mp_ai2d_set_resize_param);

STATIC mp_obj_t mp_ai2d_set_affine_param(size_t n_args, const mp_obj_t *args) {
    ai2d_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    ai2d_affine_param ap;
    ap.flag = mp_obj_is_true(args[1]);
    ap.interp_method = (uint)mp_obj_get_int(args[2]);
    ap.cord_round = (uint)mp_obj_get_int(args[3]);
    ap.bound_ind = (uint)mp_obj_get_int(args[4]);
    ap.bound_val = mp_obj_get_int(args[5]);
    ap.bound_smooth = (uint)mp_obj_get_int(args[6]);

    _kd_mpi_struct_test_f(args[7], ap.M.data, &ap.M.data_size);
    ai2d_set_affine_param(self->ai2d_, ap);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ai2d_set_affine_param_obj, 8, 8, mp_ai2d_set_affine_param);

// set dict
STATIC const mp_rom_map_elem_t ai2d_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ai2d) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&mp_ai2d_create_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_ai2d_destroy_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&mp_ai2d_destroy_obj) },
    { MP_ROM_QSTR(MP_QSTR_build), MP_ROM_PTR(&mp_ai2d_build_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_dtype), MP_ROM_PTR(&mp_ai2d_set_dtype_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_crop_param), MP_ROM_PTR(&mp_ai2d_set_crop_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_shift_param), MP_ROM_PTR(&mp_ai2d_set_shift_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pad_param), MP_ROM_PTR(&mp_ai2d_set_pad_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_resize_param), MP_ROM_PTR(&mp_ai2d_set_resize_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_affine_param), MP_ROM_PTR(&mp_ai2d_set_affine_param_obj) },
    
};

STATIC MP_DEFINE_CONST_DICT(ai2d_locals_dict, ai2d_locals_dict_table);

const mp_obj_module_t mp_module_ai2d = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&ai2d_locals_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_ai2d, mp_module_ai2d);

MP_DEFINE_CONST_OBJ_TYPE(
    ai2d_type,
    MP_QSTR_ai2d,
    MP_TYPE_FLAG_NONE,
    make_new, mp_ai2d_create,
    locals_dict, &ai2d_locals_dict
    );



// invoke
STATIC mp_obj_t mp_ai2d_run(mp_obj_t self_in, mp_obj_t inputs, mp_obj_t outputs) {
    builder_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_runtime_tensor_obj_t *input_tensor = MP_OBJ_TO_PTR(inputs);
    mp_runtime_tensor_obj_t *output_tensor = MP_OBJ_TO_PTR(outputs);
    bool flag = ai2d_run(self->builder, input_tensor->r_tensor, output_tensor->r_tensor);
    if(!flag)
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("AI2D run failed."));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(mp_ai2d_run_obj, mp_ai2d_run);

static mp_obj_t mp_ai2d_release(mp_obj_t ai2d_builder_obj) {
    builder_obj_t *self = MP_OBJ_TO_PTR(ai2d_builder_obj);
    ai2d_release(self->builder);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_ai2d_release_obj, mp_ai2d_release);


STATIC const mp_rom_map_elem_t mp_ai2d_builder_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ai2d_builder) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_ai2d_release_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&mp_ai2d_release_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&mp_ai2d_run_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_ai2d_builder_dict, mp_ai2d_builder_dict_table);

const mp_obj_module_t mp_module_ai2d_builder = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_ai2d_builder_dict,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_ai2d_builder, mp_module_ai2d_builder);

MP_DEFINE_CONST_OBJ_TYPE(
    ai2d_builder_type,
    MP_QSTR_ai2d_builder,
    MP_TYPE_FLAG_NONE,
    locals_dict, &mp_ai2d_builder_dict
);
