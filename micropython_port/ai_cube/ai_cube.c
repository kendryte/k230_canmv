#include "py/compile.h"
#include "py/runtime.h"
#include <stdlib.h>
#include <stdint.h>
#include "ndarray.h"
#include "postprocess.h"

STATIC mp_obj_t aicube_ocr_post_process(size_t n_args, const mp_obj_t *args) {

    ndarray_obj_t *data_mp_0 = MP_ROM_PTR(args[0]);
    ndarray_obj_t *data_mp_1 = MP_ROM_PTR(args[1]);
    float *data_0_tmp = data_mp_0->array;
    uint8_t *data_1_tmp = data_mp_1->array;

    float *data_0 = (float *)malloc(data_mp_0->len * sizeof(float));
    uint8_t *data_1 = (uint8_t *)malloc(data_mp_1->len * sizeof(uint8_t));

    for (int i = 0; i < data_mp_0->len; i++) 
    {
        data_0[i] = data_0_tmp[i];
    }

    for (int i = 0; i < data_mp_1->len; i++) 
    {
        data_1[i] = data_1_tmp[i];
    }

    mp_obj_list_t *kmodel_frame_size_mp = MP_OBJ_TO_PTR(args[2]);
    mp_obj_list_t *frame_size_mp = MP_OBJ_TO_PTR(args[3]);
    float threshold = mp_obj_get_float(args[4]);
    float box_thresh = mp_obj_get_float(args[5]);
    int results_size;
    FrameSize frame_size;
    FrameSize kmodel_frame_size;
    frame_size.width = mp_obj_get_int(frame_size_mp->items[0]);
    frame_size.height = mp_obj_get_int(frame_size_mp->items[1]);
    kmodel_frame_size.width = mp_obj_get_int(kmodel_frame_size_mp->items[0]);
    kmodel_frame_size.height = mp_obj_get_int(kmodel_frame_size_mp->items[1]);
    ArrayWrapper* results = ocr_post_process(frame_size, kmodel_frame_size,box_thresh,threshold, data_0, data_1,&results_size);

    mp_obj_list_t *mp_list = mp_obj_new_list(0, NULL);
    for (size_t i = 0; i < results_size; i++) {
        mp_obj_list_t *result = mp_obj_new_list(0, NULL);
        size_t ndarray_shape[4];

        ndarray_shape[0] = 1;
        ndarray_shape[1] = results[i].dimensions[1];
        ndarray_shape[2] = results[i].dimensions[2];
        ndarray_shape[3] = results[i].dimensions[0];

        ndarray_obj_t *data_output = ndarray_new_ndarray(4, ndarray_shape, NULL, NDARRAY_UINT8);

        uint8_t *data = (uint8_t*)data_output->array;
        for (int j = 0; j < results[i].dimensions[0]*results[i].dimensions[1]*results[i].dimensions[2]; j++) {
            data[j] =  results[i].data[j];
        }
        

        mp_obj_list_append(result, data_output);

        size_t ndarray_shape_1[4];

        ndarray_shape_1[3] = 8;

        ndarray_obj_t *data_output_1 = ndarray_new_ndarray(1, ndarray_shape_1, NULL, NDARRAY_FLOAT);

        float *data_1 = (float*)data_output_1->array;
        for (int j = 0; j < 8; j++) {
            data_1[j] =  results[i].coordinates[j];
        }
        mp_obj_list_append(result, data_output_1);
 
        if (results[i].data != NULL) {
            free(results[i].data);
        }
        if (results[i].dimensions != NULL) {
            free(results[i].dimensions);
        }

        mp_obj_list_append(mp_list, result);
    }

    free(results);
    free(data_0);
    free(data_1);
    return mp_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aicube_ocr_post_process_obj, 6, 6, aicube_ocr_post_process);


STATIC mp_obj_t aicube_anchorbasedet_post_process(size_t n_args, const mp_obj_t *args) {


    ndarray_obj_t *data_mp_0 = MP_ROM_PTR(args[0]);
    ndarray_obj_t *data_mp_1 = MP_ROM_PTR(args[1]);
    ndarray_obj_t *data_mp_2 = MP_ROM_PTR(args[2]);

    float *data_0_tmp = data_mp_0->array;
    float *data_1_tmp = data_mp_1->array;
    float *data_2_tmp = data_mp_2->array;

    mp_obj_list_t *kmodel_frame_size_mp = MP_OBJ_TO_PTR(args[3]);
    mp_obj_list_t *frame_size_mp = MP_OBJ_TO_PTR(args[4]);
    mp_obj_list_t *strides_mp = MP_OBJ_TO_PTR(args[5]);
    int num_class = mp_obj_get_int(args[6]);
    float ob_det_thresh = mp_obj_get_float(args[7]);
    float ob_nms_thresh = mp_obj_get_float(args[8]);
    mp_obj_list_t *anchors_mp = MP_OBJ_TO_PTR(args[9]);
    bool nms_option = mp_obj_is_true(args[10]);
    int results_size;

    float *data_0 = (float *)malloc(data_mp_0->len * sizeof(float));
    float *data_1 = (float *)malloc(data_mp_1->len * sizeof(float));
    float *data_2 = (float *)malloc(data_mp_2->len * sizeof(float));
    int strides[strides_mp->len];
    float anchors[anchors_mp->len];

    FrameSize frame_size;
    FrameSize kmodel_frame_size;
    frame_size.width = mp_obj_get_int(frame_size_mp->items[0]);
    frame_size.height = mp_obj_get_int(frame_size_mp->items[1]);
    kmodel_frame_size.width = mp_obj_get_int(kmodel_frame_size_mp->items[0]);
    kmodel_frame_size.height = mp_obj_get_int(kmodel_frame_size_mp->items[1]);

    for (int i = 0; i < data_mp_0->len; i++) 
    {
        data_0[i] = data_0_tmp[i];
    }

    for (int i = 0; i < data_mp_1->len; i++) 
    {
        data_1[i] = data_1_tmp[i];
    }

    for (int i = 0; i < data_mp_2->len; i++) 
    {
        data_2[i] = data_2_tmp[i];
    }

    for (int i = 0; i < strides_mp->len; i++) 
    {
        strides[i] = mp_obj_get_int(strides_mp->items[i]);
    }

    for (int i = 0; i < anchors_mp->len; i++) 
    {
        anchors[i] = mp_obj_get_float(anchors_mp->items[i]);
    }

    ob_det_res *results = anchorbasedet_post_process(data_0, data_1, data_2, kmodel_frame_size, frame_size, strides, num_class, ob_det_thresh, ob_nms_thresh, anchors, nms_option, &results_size);

    mp_obj_list_t *mp_list = mp_obj_new_list(0, NULL);
    for (size_t i = 0; i < results_size; i++) {
    // for (size_t i = 0; i < 2; i++) {
        mp_obj_list_t *result = mp_obj_new_list(0, NULL);
        mp_obj_list_append(result, mp_obj_new_int(results[i].label_index));
        mp_obj_list_append(result, mp_obj_new_float(results[i].score));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x2));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y2));
        mp_obj_list_append(mp_list, result);
    }

    free(results);
    free(data_0);
    free(data_1);
    free(data_2);
    return mp_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aicube_anchorbasedet_post_process_obj, 11, 11, aicube_anchorbasedet_post_process);

STATIC mp_obj_t aicube_anchorfreedet_post_process(size_t n_args, const mp_obj_t *args) {

    ndarray_obj_t *data_mp_0 = MP_ROM_PTR(args[0]);
    ndarray_obj_t *data_mp_1 = MP_ROM_PTR(args[1]);
    ndarray_obj_t *data_mp_2 = MP_ROM_PTR(args[2]);

    float *data_0_tmp = data_mp_0->array;
    float *data_1_tmp = data_mp_1->array;
    float *data_2_tmp = data_mp_2->array;

    mp_obj_list_t *kmodel_frame_size_mp = MP_OBJ_TO_PTR(args[3]);
    mp_obj_list_t *frame_size_mp = MP_OBJ_TO_PTR(args[4]);
    mp_obj_list_t *strides_mp = MP_OBJ_TO_PTR(args[5]);
    int num_class = mp_obj_get_int(args[6]);
    float ob_det_thresh = mp_obj_get_float(args[7]);
    float ob_nms_thresh = mp_obj_get_float(args[8]);
    bool nms_option = mp_obj_is_true(args[9]);
    int results_size;

    float *data_0 = (float *)malloc(data_mp_0->len * sizeof(float));
    float *data_1 = (float *)malloc(data_mp_1->len * sizeof(float));
    float *data_2 = (float *)malloc(data_mp_2->len * sizeof(float));
    int strides[strides_mp->len];

    FrameSize frame_size;
    FrameSize kmodel_frame_size;
    frame_size.width = mp_obj_get_int(frame_size_mp->items[0]);
    frame_size.height = mp_obj_get_int(frame_size_mp->items[1]);
    kmodel_frame_size.width = mp_obj_get_int(kmodel_frame_size_mp->items[0]);
    kmodel_frame_size.height = mp_obj_get_int(kmodel_frame_size_mp->items[1]);

    for (int i = 0; i < data_mp_0->len; i++) 
    {
        data_0[i] = data_0_tmp[i];
    }

    for (int i = 0; i < data_mp_1->len; i++) 
    {
        data_1[i] = data_1_tmp[i];
    }

    for (int i = 0; i < data_mp_2->len; i++) 
    {
        data_2[i] = data_2_tmp[i];
    }

    for (int i = 0; i < strides_mp->len; i++) 
    {
        strides[i] = mp_obj_get_int(strides_mp->items[i]);
    }

    ob_det_res *results = anchorfreedet_post_process(data_0, data_1, data_2, kmodel_frame_size, frame_size, strides, num_class, ob_det_thresh, ob_nms_thresh, nms_option, &results_size);

    mp_obj_list_t *mp_list = mp_obj_new_list(0, NULL);
    for (size_t i = 0; i < results_size; i++) {
    // for (size_t i = 0; i < 2; i++) {
        mp_obj_list_t *result = mp_obj_new_list(0, NULL);
        mp_obj_list_append(result, mp_obj_new_int(results[i].label_index));
        mp_obj_list_append(result, mp_obj_new_float(results[i].score));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x2));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y2));
        mp_obj_list_append(mp_list, result);
    }

    free(results);
    free(data_0);
    free(data_1);
    free(data_2);
    return mp_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aicube_anchorfreedet_post_process_obj, 10, 10, aicube_anchorfreedet_post_process);

STATIC mp_obj_t aicube_gfldet_post_process(size_t n_args, const mp_obj_t *args) {

    ndarray_obj_t *data_mp_0 = MP_ROM_PTR(args[0]);
    ndarray_obj_t *data_mp_1 = MP_ROM_PTR(args[1]);
    ndarray_obj_t *data_mp_2 = MP_ROM_PTR(args[2]);

    float *data_0_tmp = data_mp_0->array;
    float *data_1_tmp = data_mp_1->array;
    float *data_2_tmp = data_mp_2->array;

    mp_obj_list_t *kmodel_frame_size_mp = MP_OBJ_TO_PTR(args[3]);
    mp_obj_list_t *frame_size_mp = MP_OBJ_TO_PTR(args[4]);
    mp_obj_list_t *strides_mp = MP_OBJ_TO_PTR(args[5]);
    int num_class = mp_obj_get_int(args[6]);
    float ob_det_thresh = mp_obj_get_float(args[7]);
    float ob_nms_thresh = mp_obj_get_float(args[8]);
    bool nms_option = mp_obj_is_true(args[9]);
    int results_size;

    float *data_0 = (float *)malloc(data_mp_0->len * sizeof(float));
    float *data_1 = (float *)malloc(data_mp_1->len * sizeof(float));
    float *data_2 = (float *)malloc(data_mp_2->len * sizeof(float));
    int strides[strides_mp->len];

    FrameSize frame_size;
    FrameSize kmodel_frame_size;
    frame_size.width = mp_obj_get_int(frame_size_mp->items[0]);
    frame_size.height = mp_obj_get_int(frame_size_mp->items[1]);
    kmodel_frame_size.width = mp_obj_get_int(kmodel_frame_size_mp->items[0]);
    kmodel_frame_size.height = mp_obj_get_int(kmodel_frame_size_mp->items[1]);

    for (int i = 0; i < data_mp_0->len; i++) 
    {
        data_0[i] = data_0_tmp[i];
    }

    for (int i = 0; i < data_mp_1->len; i++) 
    {
        data_1[i] = data_1_tmp[i];
    }

    for (int i = 0; i < data_mp_2->len; i++) 
    {
        data_2[i] = data_2_tmp[i];
    }

    for (int i = 0; i < strides_mp->len; i++) 
    {
        strides[i] = mp_obj_get_int(strides_mp->items[i]);
    }

    ob_det_res *results = gfldet_post_process(data_0, data_1, data_2, kmodel_frame_size, frame_size, strides, num_class, ob_det_thresh, ob_nms_thresh, nms_option, &results_size);

    mp_obj_list_t *mp_list = mp_obj_new_list(0, NULL);
    for (size_t i = 0; i < results_size; i++) {
    // for (size_t i = 0; i < 2; i++) {
        mp_obj_list_t *result = mp_obj_new_list(0, NULL);
        mp_obj_list_append(result, mp_obj_new_int(results[i].label_index));
        mp_obj_list_append(result, mp_obj_new_float(results[i].score));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y1));
        mp_obj_list_append(result, mp_obj_new_int(results[i].x2));
        mp_obj_list_append(result, mp_obj_new_int(results[i].y2));
        mp_obj_list_append(mp_list, result);
    }

    free(results);
    free(data_0);
    free(data_1);
    free(data_2);
    return mp_list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aicube_gfldet_post_process_obj, 10, 10, aicube_gfldet_post_process);

STATIC mp_obj_t aicube_seg_post_process(size_t n_args, const mp_obj_t *args) {
    ndarray_obj_t *data_mp = MP_ROM_PTR(args[0]);
    float *data = data_mp->array;
    int num_class = mp_obj_get_int(args[1]);
    mp_obj_list_t *ori_shape_mp = MP_OBJ_TO_PTR(args[2]);
    mp_obj_list_t *dst_shape_mp = MP_OBJ_TO_PTR(args[3]);

    FrameSize ori_shape;
    FrameSize dst_shape;

    ori_shape.height = mp_obj_get_int(ori_shape_mp->items[0]);
    ori_shape.width = mp_obj_get_int(ori_shape_mp->items[1]);
    dst_shape.height = mp_obj_get_int(dst_shape_mp->items[0]);
    dst_shape.width = mp_obj_get_int(dst_shape_mp->items[1]);

    uint8_t *result = seg_post_process(data, num_class, ori_shape, dst_shape);

    size_t ndarray_shape[4];
    ndarray_shape[1] = dst_shape.height;
    ndarray_shape[2] = dst_shape.width;
    ndarray_shape[3] = 4;
    ndarray_obj_t *result_obj = ndarray_new_ndarray(3, ndarray_shape, NULL, NDARRAY_UINT8);

    uint8_t *result_data = (uint8_t *)result_obj->array;
    for (int i=0; i<dst_shape.height*dst_shape.width*4; i++)
    {
        result_data[i] = result[i];
    }

    free(result);
    return MP_OBJ_FROM_PTR(result_obj);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(aicube_seg_post_process_obj, 4, 4, aicube_seg_post_process);

STATIC const mp_rom_map_elem_t aicube_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_aicube) },
    { MP_ROM_QSTR(MP_QSTR_ocr_post_process), MP_ROM_PTR(&aicube_ocr_post_process_obj) },
    { MP_ROM_QSTR(MP_QSTR_anchorbasedet_post_process), MP_ROM_PTR(&aicube_anchorbasedet_post_process_obj) },
    { MP_ROM_QSTR(MP_QSTR_anchorfreedet_post_process), MP_ROM_PTR(&aicube_anchorfreedet_post_process_obj) },
    { MP_ROM_QSTR(MP_QSTR_gfldet_post_process), MP_ROM_PTR(&aicube_gfldet_post_process_obj) },
    { MP_ROM_QSTR(MP_QSTR_seg_post_process), MP_ROM_PTR(&aicube_seg_post_process_obj) },
};

STATIC MP_DEFINE_CONST_DICT(aicube_globals, aicube_globals_table);

const mp_obj_module_t aicube = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&aicube_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_aicube, aicube);
