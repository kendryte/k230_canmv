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
#ifndef _AIDEMO_WRAP_H_
#define _AIDEMO_WRAP_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "aidemo_type.h"

//for common
typedef struct generic_array generic_array;
typedef struct FrameSize FrameSize;
typedef struct CHWSize CHWSize;
//for cv
typedef struct cv_and_ndarray_convert_info cv_and_ndarray_convert_info;
typedef struct BoxPoint8 BoxPoint8;
typedef struct ArrayWrapperMat1 ArrayWrapperMat1;
//for face det
typedef struct Bbox Bbox;
typedef struct SparseLandmarks SparseLandmarks;
typedef struct FaceDetectionInfoVector FaceDetectionInfoVector;
//for object segment
typedef struct SegOutput SegOutput;
typedef struct SegOutputs SegOutputs;
//for person kp det
typedef struct PersonKPOutput PersonKPOutput;
// for kws 
typedef struct feature_pipeline feature_pipeline;
typedef struct processed_feat processed_feat;
// for nanotracker
typedef struct Tracker_box Tracker_box;
typedef struct Tracker_box_center Tracker_box_center;
// for tts_zh
typedef struct TtsZh TtsZh;
typedef struct TtsZhOutput TtsZhOutput;

#ifdef __cplusplus
extern "C" {
#endif
    //for cv
    void invert_affine_transform(float* data,cv_and_ndarray_convert_info* info);
    void draw_polylines(cv_and_ndarray_convert_info* in_info,generic_array* pts,bool is_closed,generic_array* color,int thickness,int line_type,int shift);
    void draw_contours(cv_and_ndarray_convert_info* in_info,generic_array* pts,int contour_idx,generic_array* color,int thickness,int line_type);
    float* mask_resize(float* dest, FrameSize ori_shape, FrameSize tag_shape);
    void draw_mesh(cv_and_ndarray_convert_info *in_info, generic_array* p_vertices);
    //for ocr rec
    ArrayWrapperMat1* ocr_rec_pre_process(uint8_t* data, FrameSize ori_shape, BoxPoint8* boxpoint8, int box_cnt);

    //for face det
    FaceDetectionInfoVector* face_detetion_post_process(float obj_thresh,float nms_thresh,int net_len,float* anchors,FrameSize* frame_size,float** p_outputs);
    //for face parse
    void face_parse_post_process(cv_and_ndarray_convert_info* in_info,FrameSize* ai_img_shape,FrameSize* osd_img_shape,int net_len,Bbox* bbox,CHWSize* model_out_shape,float* p_outputs);
    //for face mesh
    void face_mesh_post_process(Bbox roi,generic_array* p_vertices);
    //for licence det
    BoxPoint8* licence_det_post_process(float* p_outputs_0,float* p_outputs_1,float* p_outputs_2,float* p_outputs_3,float* p_outputs_4,float* p_outputs_5,float* p_outputs_6,float* p_outputs_7,float* p_outputs_8,FrameSize frame_size,FrameSize kmodel_frame_size,float obj_thresh,float nms_thresh,int* box_cnt);
    //for object segment
    SegOutputs object_seg_post_process(float *data_0, float *data_1, FrameSize frame_size, FrameSize kmodel_frame_size, FrameSize display_frame_size, float conf_thres, float nms_thres, float mask_thres, int *box_cnt);
    //for person kp det
    PersonKPOutput* person_kp_postprocess(float *data, FrameSize frame_size, FrameSize kmodel_frame_size, float obj_thresh, float nms_thresh, int *box_cnt);
    //for kws
    feature_pipeline *feature_pipeline_create();
    void release_preprocess_class(feature_pipeline *fp);
    void release_final_feats(float* feats);
    void wav_preprocess(feature_pipeline *fp, float *wav, size_t wav_length, float* final_feats);
    void release_preprocess_class(feature_pipeline *fp);
    //for eye_gaze
    void eye_gaze_post_process(float** p_outputs_,float* pitch,float* yaw);
    //for nanotracker
    Tracker_box_center nanotracker_post_process(float* output_0, float* output_1, FrameSize sensor_size, float thresh, float* center_xy_wh, int crop_size, float CONTEXT_AMOUNT);
    //for tts_zh
    TtsZh *ttszh_create();
    void ttszh_destroy(TtsZh* ttszh_);
    void ttszh_init(TtsZh* ttszh_,const char* dictfile,const char* phasefile,const char* mapfile);
    TtsZhOutput* tts_zh_frontend_preprocess(TtsZh* ttszh_,const char* text);
    void tts_save_wav(float* wav_data,int wav_len,const char* wav_filename,int sample_rate);
#ifdef __cplusplus
}
#endif

#endif // _AIDEMO_WRAP_H_