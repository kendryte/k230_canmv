#include "nncase_wrap.h"
#include "nncase_type.h"
#include "nncase/runtime/interpreter.h"
#include "nncase/runtime/runtime_tensor.h"
#include "nncase/functional/ai2d/ai2d_builder.h"
#include "nncase/runtime/k230/gnne_tile_utils.h"
#include "nncase/runtime/util.h"
#include "nncase/runtime/result.h"
#include "nncase/version.h"
#include <fstream>
#include <iostream>
#include <iterator>

// define C struct of c++ class


// kpu class
struct interpreter
{
    nncase::runtime::interpreter *interp;
};

struct runtime_tensor
{
    nncase::runtime::runtime_tensor *r_tensor;
};


struct dtype
{
    nncase::typecode_t *d_t;
};

struct dims
{
    nncase::dims_t *d_dims;
};

// ai2d class
struct ai2d
{
    nncase::runtime::k230::ai2d_datatype_t ai2d_datatype;
    nncase::runtime::k230::ai2d_crop_param_t ai2d_crop_param;
    nncase::runtime::k230::ai2d_shift_param_t ai2d_shift_param;
    nncase::runtime::k230::ai2d_pad_param_t ai2d_pad_param;
    nncase::runtime::k230::ai2d_resize_param_t ai2d_resize_param;
    nncase::runtime::k230::ai2d_affine_param_t ai2d_affine_param;
};

struct ai2d_builder
{
    nncase::F::k230::ai2d_builder *builder;
};

// utils
tensor_desc get_tensor_desc_info(tensor_desc *data)
{
    return {.datatype = data->datatype, .start = data->start, .size = data->size};
}

char get_dtype_for_mp(nncase::typecode_t dtype)
{
    switch (dtype)
    {
    case nncase::typecode_t::dt_boolean:
        return '?';
    case nncase::typecode_t::dt_int8:
        return 'b';
    case nncase::typecode_t::dt_uint8:
        return 'B';
    case nncase::typecode_t::dt_int16:
        return 'h';
    case nncase::typecode_t::dt_uint16:
        return 'H';
    case nncase::typecode_t::dt_int32:
        return 'i';
    case nncase::typecode_t::dt_uint32:
        return 'I';
    case nncase::typecode_t::dt_int64:
        return 'l';
    case nncase::typecode_t::dt_uint64:
        return 'L';
    case nncase::typecode_t::dt_float32:
        return 'f';
    case nncase::typecode_t::dt_float64:
        return 'd';
    default:
        throw std::runtime_error("Unsupported data type.");
    }
}

int mp_dtype_to_nncase(char data_type)
{
    switch (data_type)
    {
    case '?':
        return 0; // nncase::typecode_t::dt_boolean;
    case 'b':
        return 2; // nncase::typecode_t::dt_int8;
    case 'B':
        return 6; // nncase::typecode_t::dt_uint8;
    case 'h':
        return 3; // nncase::typecode_t::dt_int16;
    case 'H':
        return 7; // nncase::typecode_t::dt_uint16;
    case 'i':
        return 4; // nncase::typecode_t::dt_int32;
    case 'I':
        return 8; // nncase::typecode_t::dt_uint32;
    case 'l':
        return 5; // nncase::typecode_t::dt_int64;
    case 'L':
        return 9; // nncase::typecode_t::dt_uint64;
    case 'f':
        return 11; // nncase::typecode_t::dt_float32;
    case 'd':
        return 12; // nncase::typecode_t::dt_float64;
    default:
        return -1;
    }
}

void shrink_memory_pool()
{
    nncase::runtime::shrink_memory_pool();
}


// func 
Kpu *Kpu_create()
{
    Kpu *kpu = new Kpu;
    kpu->interp = new nncase::runtime::interpreter();
    return kpu;
}

void Kpu_destroy(Kpu* p) {
    delete p->interp;
    p->interp = nullptr;
    delete p;
    p = nullptr;
}

bool Kpu_run(Kpu* p){
    auto state = p->interp->run();
    return state.is_ok();
}

bool Kpu_load_kmodel_path(Kpu* p, const char* path)
{
    std::ifstream ifs(path, std::ios::binary);
    auto state = p->interp->load_model(ifs);
    return state.is_ok();
}

bool Kpu_load_kmodel_buffer(Kpu* p, char* buffer, size_t size)
{
    gsl::span<const gsl::byte> span((gsl::byte*)buffer, size);
    auto state = p->interp->load_model(span);
    return state.is_ok();
}

bool Kpu_set_input_tensor(Kpu* p, size_t index, runtime_tensor *tensor)
{
    auto state = p->interp->input_tensor(index, *tensor->r_tensor); //.expect("kpu set input tensor failed.");
    return state.is_ok();
}

runtime_tensor* Kpu_get_input_tensor(Kpu* p, size_t index)
{
    runtime_tensor *tensor = new runtime_tensor();
    auto data = p->interp->input_tensor(index).expect("kpu get input tensor failed.");
    tensor->r_tensor = new nncase::runtime::runtime_tensor(data.impl());
    return tensor;
}

bool Kpu_set_output_tensor(Kpu* p, size_t index, runtime_tensor *tensor)
{
    auto state = p->interp->output_tensor(index, *tensor->r_tensor); //.expect("kpu set output tensor failed.");
    return state.is_ok();
}

runtime_tensor* Kpu_get_output_tensor(Kpu* p, size_t index)
{
    runtime_tensor *tensor = new runtime_tensor();
    auto data = p->interp->output_tensor(index).expect("kpu get input tensor failed.");
    tensor->r_tensor = new nncase::runtime::runtime_tensor(data.impl());
    return tensor;
}

size_t Kpu_inputs_size(Kpu* p)
{
    return p->interp->inputs_size();
}

size_t Kpu_outputs_size(Kpu* p)
{
    return p->interp->outputs_size();
}

tensor_desc Kpu_get_input_desc(Kpu* p, size_t index)
{
    nncase::runtime::tensor_desc desc = p->interp->input_desc(index);
    tensor_desc d = {.datatype = int(desc.datatype), .start = desc.start, .size = desc.size};
    return d;
}

tensor_desc Kpu_get_output_desc(Kpu* p, size_t index)
{
    nncase::runtime::tensor_desc desc = p->interp->output_desc(index);
    tensor_desc d ={.datatype = desc.datatype, .start = desc.start, .size = desc.size };
    return d;
}

runtime_tensor* from_numpy(int dtype, finite_data shape, void* data, uint64_t phy_addr)
{
    if(dtype == -1)
        throw std::runtime_error("Unsupported data type.");
    runtime_tensor *tensor = new runtime_tensor;
    std::vector<int32_t> shape_data(shape.data_size, 0);

    for (int i = 0; i < shape.data_size; i++)
    {
        shape_data[i] = (int)shape.data[i];
    }

    size_t data_bytes = std::accumulate(shape_data.begin(), shape_data.end(), 1, std::multiplies<int32_t>()) * typecode_bytes((nncase::typecode_t)dtype);
    nncase::dims_t shape_(shape_data.begin(), shape_data.end());
    
    bool copy_flag = false;
    if (phy_addr == 0)
        copy_flag = true;
        
    auto local_data = nncase::runtime::host_runtime_tensor::create(
                          (nncase::typecode_t)dtype, shape_, {(gsl::byte *)data, data_bytes},
                          copy_flag, nncase::runtime::host_runtime_tensor::pool_shared, phy_addr)
                          .expect("cannot create input tensor");
    nncase::runtime::host_runtime_tensor::sync(local_data, nncase::runtime::sync_op_t::sync_write_back, true).expect("sync write_back failed");
    tensor->r_tensor = new nncase::runtime::runtime_tensor(local_data.impl());
    return tensor;
}

void to_numpy(runtime_tensor * tensor, rt_to_ndarray_info *info)
{
    info->dtype_ = get_dtype_for_mp(tensor->r_tensor->datatype());
    auto shape = tensor->r_tensor->shape();
    info->ndim_ = shape.size();
    info->len_ = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
    
    for(int i = 0; i < info->ndim_; i++)
    {
        info->shape_[i] = shape[i];
        info->strides_[i] = tensor->r_tensor->strides()[i];
    }

    info->data_ =std::move((void *)nncase::runtime::host_runtime_tensor::map(*tensor->r_tensor, nncase::runtime::map_access_t::map_read).unwrap().buffer().data());

}


ai2d* ai2d_create()
{
    ai2d *p = new ai2d;
    // datatype
    p->ai2d_datatype.src_format = nncase::runtime::k230::ai2d_format::NCHW_FMT;
    p->ai2d_datatype.dst_format = nncase::runtime::k230::ai2d_format::NCHW_FMT;
    p->ai2d_datatype.src_type = nncase::typecode_t::dt_uint8;
    p->ai2d_datatype.dst_type = nncase::typecode_t::dt_uint8;
    // crop param
    p->ai2d_crop_param.crop_flag = false;
    p->ai2d_crop_param.start_x = 0;
    p->ai2d_crop_param.start_y = 0;
    p->ai2d_crop_param.height = 0;
    p->ai2d_crop_param.width = 0;
    //shift param
    p->ai2d_shift_param.shift_flag = false;
    p->ai2d_shift_param.shift_val = 0;
    // pad param
    p->ai2d_pad_param.pad_flag = false;
    p->ai2d_pad_param.paddings = {{0,0}, {0,0}, {0,0}, {0,0}};
    p->ai2d_pad_param.pad_mode = nncase::runtime::k230::ai2d_pad_mode::constant;
    p->ai2d_pad_param.pad_val = {0, 0, 0};
    // resize param
    p->ai2d_resize_param.resize_flag = false;
    p->ai2d_resize_param.interp_method = nncase::runtime::k230::ai2d_interp_method::tf_nearest;
    p->ai2d_resize_param.interp_mode = nncase::runtime::k230::ai2d_interp_mode::none;
    // affine param
    p->ai2d_affine_param.affine_flag = false;
    p->ai2d_affine_param.interp_method = nncase::runtime::k230::ai2d_interp_method::tf_nearest;
    p->ai2d_affine_param.bound_ind = 0;
    p->ai2d_affine_param.bound_val = 0;
    p->ai2d_affine_param.bound_smooth = 0;
    p->ai2d_affine_param.cord_round = 0;
    p->ai2d_affine_param.M = {0, 0, 0, 0, 0, 0};
    return p;
}

void ai2d_destroy(ai2d *p)
{   
    delete p;
    p = nullptr;
}

m_builder* ai2d_build(ai2d *p, finite_data input_shape, finite_data output_shape)
{
    std::vector<size_t> in_shape_(input_shape.data_size,0);
    std::vector<size_t> out_shape_(output_shape.data_size,0);

    for (int i = 0; i< input_shape.data_size; i++)
    {
        in_shape_[i] = (size_t)input_shape.data[i];
    }
    for (int i = 0; i< output_shape.data_size; i++)
    {
        out_shape_[i] = (size_t)output_shape.data[i];
    }

    nncase::dims_t in_shape(in_shape_.begin(), in_shape_.end());
    nncase::dims_t out_shape(out_shape_.begin(), out_shape_.end());

    if(in_shape_[3]<=32 && p->ai2d_pad_param.paddings[3].before>0)
        throw std::runtime_error("[ERROR] ai2d pad: input width is <=32, the left pad should not be set. You can set the right pad first, then set the left pad.");
    m_builder *mbuilder = new m_builder;
    mbuilder->builder = new nncase::F::k230::ai2d_builder(in_shape,
                                                          out_shape,
                                                          p->ai2d_datatype,
                                                          p->ai2d_crop_param,
                                                          p->ai2d_shift_param,
                                                          p->ai2d_pad_param,
                                                          p->ai2d_resize_param,
                                                          p->ai2d_affine_param);

    mbuilder->builder->build_schedule().expect("ai2d build schedule failed.");
    return mbuilder;
}

bool ai2d_run(m_builder* p, runtime_tensor *in_tensor, runtime_tensor *out_tensor)
{
    auto state = p->builder->invoke(*in_tensor->r_tensor, *out_tensor->r_tensor);
    return state.is_ok();
}

// set ai2d args
void ai2d_set_dtype(ai2d *p, ai2d_dtype_param dtype_param)
{
    p->ai2d_datatype.src_format = (enum nncase::runtime::k230::ai2d_format)dtype_param.src_format;
    p->ai2d_datatype.dst_format = (enum nncase::runtime::k230::ai2d_format)dtype_param.dst_format;
    p->ai2d_datatype.src_type = (nncase::typecode_t)dtype_param.src_type;
    p->ai2d_datatype.dst_type = (nncase::typecode_t)dtype_param.dst_type;
}

void ai2d_set_crop_param(ai2d *p, ai2d_crop_param crop_params)
{
    p->ai2d_crop_param.crop_flag = crop_params.flag;
    p->ai2d_crop_param.start_x = crop_params.start_x;
    p->ai2d_crop_param.start_y = crop_params.start_y;
    p->ai2d_crop_param.width = crop_params.width;
    p->ai2d_crop_param.height = crop_params.height;
}

void ai2d_set_shift_param(ai2d *p, ai2d_shift_param shift_params)
{
    p->ai2d_shift_param.shift_flag = shift_params.flag;
    p->ai2d_shift_param.shift_val = shift_params.shift_value;
}

void ai2d_set_pad_param(ai2d *p, ai2d_pad_param pad_params)
{
    p->ai2d_pad_param.pad_flag = pad_params.flag;
    std::vector<int32_t> paddings_index( pad_params.paddings.data,  pad_params.paddings.data + pad_params.paddings.data_size);

    p->ai2d_pad_param.paddings = {{paddings_index[0], paddings_index[1]},
                                   {paddings_index[2], paddings_index[3]},
                                   {paddings_index[4], paddings_index[5]},
                                   {paddings_index[6], paddings_index[7]}};
    p->ai2d_pad_param.pad_mode = (enum nncase::runtime::k230::ai2d_pad_mode)pad_params.pad_mode;
    p->ai2d_pad_param.pad_val = std::vector<int32_t>(pad_params.pad_value.data, pad_params.pad_value.data + pad_params.pad_value.data_size);
}

void ai2d_set_resize_param(ai2d *p, ai2d_resize_param resize_params)
{
    p->ai2d_resize_param.resize_flag = resize_params.flag;
    p->ai2d_resize_param.interp_method = (enum nncase::runtime::k230::ai2d_interp_method)resize_params.interp_method;
    p->ai2d_resize_param.interp_mode = (enum nncase::runtime::k230::ai2d_interp_mode)resize_params.interp_mode;
}

void ai2d_set_affine_param(ai2d *p, ai2d_affine_param affine_params)
{
    p->ai2d_affine_param.affine_flag = affine_params.flag;
    p->ai2d_affine_param.interp_method = (enum nncase::runtime::k230::ai2d_interp_method)affine_params.interp_method;
    p->ai2d_affine_param.cord_round = affine_params.cord_round;
    p->ai2d_affine_param.bound_ind = affine_params.bound_ind;
    p->ai2d_affine_param.bound_val = affine_params.bound_val;
    p->ai2d_affine_param.bound_smooth = affine_params.bound_smooth;
    p->ai2d_affine_param.M = std::vector<float>(affine_params.M.data, affine_params.M.data+affine_params.M.data_size);
}

void runtime_tensor_release(runtime_tensor *tensor)
{
    delete tensor->r_tensor;
    tensor->r_tensor = nullptr;
    delete tensor;
    tensor = nullptr;
}

void ai2d_release(m_builder *p)
{
    delete p->builder;
    p->builder = nullptr;
    delete p;
    p = nullptr;
}

char* version()
{
    return NNCASE_VERSION;
}