#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef struct runtime_tensor runtime_tensor;
typedef struct tensor_desc tensor_desc;
typedef struct rt_to_ndarray_info rt_to_ndarray_info;
typedef struct interpreter Kpu;
// typedef struct dtype dtype;
typedef struct ai2d ai2d;
typedef struct dims dims;
typedef struct ai2d_dtype_param ai2d_dtype_param;
typedef struct ai2d_crop_param ai2d_crop_param;
typedef struct ai2d_shift_param ai2d_shift_param;
typedef struct ai2d_pad_param ai2d_pad_param;
typedef struct ai2d_resize_param ai2d_resize_param;
typedef struct ai2d_affine_param ai2d_affine_param;
typedef struct finite_data finite_data;
typedef struct ai2d_builder m_builder;

#ifdef __cplusplus
extern "C" {
#endif
    Kpu* Kpu_create();
    void Kpu_destroy(Kpu *p);
    bool Kpu_run(Kpu *p);
    bool Kpu_load_kmodel_path(Kpu *p, const char *path);
    bool Kpu_load_kmodel_buffer(Kpu *p, char *buffer, size_t size);
    bool Kpu_set_input_tensor(Kpu *p, size_t index, runtime_tensor *tensor);
    runtime_tensor* Kpu_get_input_tensor(Kpu *p, size_t index);
    bool Kpu_set_output_tensor(Kpu *p, size_t index, runtime_tensor *tensor);
    runtime_tensor* Kpu_get_output_tensor(Kpu *p, size_t index);
    size_t Kpu_inputs_size(Kpu *p);
    size_t Kpu_outputs_size(Kpu *p);
    tensor_desc Kpu_get_input_desc(Kpu *p, size_t index);
    tensor_desc Kpu_get_output_desc(Kpu *p, size_t index);
    runtime_tensor* from_numpy(int dtype, finite_data shape, void* data, uint64_t phy_addr);
    void to_numpy(runtime_tensor* tensor, rt_to_ndarray_info *info);
    ai2d *ai2d_create();
    void ai2d_destroy(ai2d *p);
    m_builder* ai2d_build(ai2d *p, finite_data input_shape, finite_data output_shape);
    bool ai2d_run(m_builder *p, runtime_tensor* input_tensor, runtime_tensor* output_tensor);
    
    void ai2d_set_dtype(ai2d *p, ai2d_dtype_param dtype);
    void ai2d_set_crop_param(ai2d *p, ai2d_crop_param crop_params);
    void ai2d_set_shift_param(ai2d *p, ai2d_shift_param shift_params);
    void ai2d_set_pad_param(ai2d *p, ai2d_pad_param pad_params);
    void ai2d_set_resize_param(ai2d *p, ai2d_resize_param resize_params);
    void ai2d_set_affine_param(ai2d *p, ai2d_affine_param affine_params);
    
    int mp_dtype_to_nncase(char data_type);
    void runtime_tensor_release(runtime_tensor *tensor);
    void ai2d_release(m_builder *p);
    void shrink_memory_pool();
    
    char* version();
#ifdef __cplusplus
}
#endif