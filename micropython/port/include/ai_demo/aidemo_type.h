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
#ifndef _AIDEMO_TYPE_H_
#define _AIDEMO_TYPE_H_
#include "py/obj.h"
#include "aidemo_wrap.h"

//*****************************for common*****************************
struct FrameSize
{
    size_t width;  // 宽
    size_t height; // 高
};

struct CHWSize
{
    size_t channel; // 通道
    size_t height;  // 高
    size_t width;   // 宽
};

struct generic_array{
    void *data_;      
    size_t length_;   
};

//*****************************for cv*****************************
struct cv_and_ndarray_convert_info
{
    uint8_t dtype_;
    uint8_t ndim_;
    size_t len_;
    size_t shape_[3];           //ULAB_MAX_DIMS
    size_t strides_[3];
    void *data_;
};

struct BoxPoint8
{
    float points8[8];
};

struct ArrayWrapperMat1
{
    uint8_t* data;         // 指向数组的指针
    FrameSize framesize;   // 数组的维度信息
    float coordinates[8];   // 画图的四个点的位置
};

//*****************************for face det*****************************
struct Bbox
{
    float x; // 人脸检测框的左顶点x坐标
    float y; // 人脸检测框的左顶点x坐标
    float w;
    float h;
};

struct SparseLandmarks
{
    float points[10]; // 人脸五官点,依次是图片的左眼（x,y）、右眼（x,y）,鼻子（x,y）,左嘴角（x,y）,右嘴角
};

struct FaceDetectionInfoVector
{
    size_t vec_len;
    Bbox *bbox;                  // 人脸检测框
    SparseLandmarks *sparse_kps; // 人脸五官关键点
    float *score;                // 人脸检测框置信度
};

//*****************************for object segment*****************************
struct SegOutput {
	int id;             //结果类别id
	float confidence;   //结果置信度
	int box[4];       //矩形框
};

struct SegOutputs {
    SegOutput *segOutput;
    uint8_t *masks_results;
};

//*****************************for person keypoint detect**********************
struct PersonKPOutput {
	float confidence;   //结果置信度
	int box[4];       //矩形框  x1 y1 x2 y2
    float kps[17][3];     //关键点 向量
};

//*****************************for nanotracker**********************
struct Tracker_box
{
    int x;
    int y;
    int w;
    int h; 
    float score;
};

struct Tracker_box_center
{
    Tracker_box tracker_box;
    float center_xy_wh[4];
    bool exist;
};

//*****************************for tts_zh*****************************
struct TtsZhOutput
{
    float* data;
    size_t size;
    int* len_data;
    size_t len_size;
};
#endif // _AIDEMO_TYPE_H_