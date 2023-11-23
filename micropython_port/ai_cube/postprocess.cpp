#include "postprocess.h"
#include <opencv2/imgproc.hpp>
#include <vector>
#include <string>
#include <string.h>
#include <cmath>
#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>

// #include <opencv/cv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;

#define REG_MAX 16
#define STRIDE_NUM 3
#define STAGE_NUM 3

vector<ob_det_res> anchorbasedet_decode_infer(float* data, FrameSize kmodel_frame_size, FrameSize frame_size, int stride, int num_class, float ob_det_thresh, float anchors[3][2])
{
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    std::vector<ob_det_res> result;
    int grid_size_w = kmodel_frame_size.width / stride;
    int grid_size_h = kmodel_frame_size.height / stride;
    int one_rsize = num_class + 5;
    float cx, cy, w, h;

    for (int shift_y = 0; shift_y < grid_size_h; shift_y++)
    {
        for (int shift_x = 0; shift_x < grid_size_w; shift_x++)
        {
            int loc = shift_x + shift_y * grid_size_w;
            for (int i = 0; i < 3; i++)
            {
                float* record = data + (loc * 3 + i) * one_rsize;
                float* cls_ptr = record + 5;
                for (int cls = 0; cls < num_class; cls++)
                {
                    float score = (cls_ptr[cls]) * (record[4]);
                    if (score > ob_det_thresh)
                    {
                        cx = ((record[0]) * 2.f - 0.5f + (float)shift_x) * (float)stride;
                        cy = ((record[1]) * 2.f - 0.5f + (float)shift_y) * (float)stride;
                        w = pow((record[2]) * 2.f, 2) * anchors[i][0];
                        h = pow((record[3]) * 2.f, 2) * anchors[i][1];
                        cx -= ((kmodel_frame_size.width - frame_size.width * gain) / 2);
                        cy -= ((kmodel_frame_size.height - frame_size.height * gain) / 2);
                        cx /= gain;
                        cy /= gain;
                        w /= gain;
                        h /= gain;
                        ob_det_res box;
                        box.x1 = std::max(0, std::min(int(frame_size.width), int(cx - w / 2.f)));
                        box.y1 = std::max(0, std::min(int(frame_size.height), int(cy - h / 2.f)));
                        box.x2 = std::max(0, std::min(int(frame_size.width), int(cx + w / 2.f)));
                        box.y2 = std::max(0, std::min(int(frame_size.height), int(cy + h / 2.f)));
                        box.score = score;
                        box.label_index = cls;
                        result.push_back(box);
                    }
                }
            }
        }
    }
    return result;
}

vector<vector<ob_det_res>> anchorbasedet_decode_infer_class(float* data, FrameSize kmodel_frame_size, FrameSize frame_size, int stride, int num_class, float ob_det_thresh, float anchors[3][2])
{
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    std::vector<std::vector<ob_det_res>> result;
    for (int i = 0; i < num_class; i++)
    {
        result.push_back(vector<ob_det_res>());
    }
    int grid_size_w = kmodel_frame_size.width / stride;
    int grid_size_h = kmodel_frame_size.height / stride;
    int one_rsize = num_class + 5;
    float cx, cy, w, h;
    for (int shift_y = 0; shift_y < grid_size_h; shift_y++)
    {
        for (int shift_x = 0; shift_x < grid_size_w; shift_x++)
        {
            int loc = shift_x + shift_y * grid_size_w;
            for (int i = 0; i < 3; i++)
            {
                float* record = data + (loc * 3 + i) * one_rsize;
                float* cls_ptr = record + 5;
                for (int cls = 0; cls < num_class; cls++)
                {
                    float score = (cls_ptr[cls]) * (record[4]);
                    if (score > ob_det_thresh)
                    {
                        cx = ((record[0]) * 2.f - 0.5f + (float)shift_x) * (float)stride;
                        cy = ((record[1]) * 2.f - 0.5f + (float)shift_y) * (float)stride;
                        w = pow((record[2]) * 2.f, 2) * anchors[i][0];
                        h = pow((record[3]) * 2.f, 2) * anchors[i][1];
                        cx -= ((kmodel_frame_size.width - frame_size.width * gain) / 2);
                        cy -= ((kmodel_frame_size.height - frame_size.height * gain) / 2);
                        cx /= gain;
                        cy /= gain;
                        w /= gain;
                        h /= gain;
                        ob_det_res box;
                        box.x1 = std::max(0, std::min(int(frame_size.width), int(cx - w / 2.f)));
                        box.y1 = std::max(0, std::min(int(frame_size.height), int(cy - h / 2.f)));
                        box.x2 = std::max(0, std::min(int(frame_size.width), int(cx + w / 2.f)));
                        box.y2 = std::max(0, std::min(int(frame_size.height), int(cy + h / 2.f)));
                        box.score = score;
                        box.label_index = cls;
                        result[cls].push_back(box);
                    }
                }
            }
        }
    }
    return result;
}


vector<ob_det_res> anchorfreedet_decode_infer(float* data, FrameSize kmodel_frame_size, FrameSize frame_size, int stride, int num_class, float ob_det_thresh)
{
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    std::vector<ob_det_res> result;
    int grid_size_w = kmodel_frame_size.width / stride;
    int grid_size_h = kmodel_frame_size.height / stride;
    int one_rsize = num_class + 5;
    float cx, cy, w, h;
    for (int shift_y = 0; shift_y < grid_size_h; shift_y++)
    {
        for (int shift_x = 0; shift_x < grid_size_w; shift_x++)
        {

            int loc = shift_x + shift_y * grid_size_w;
            for (int i = 0; i < 1; i++)
            {
                float* record = data + (loc + i) * one_rsize;
                float* cls_ptr = record + 5;
                for (int cls = 0; cls < num_class; cls++)
                {
                    // float score = sigmoid(record[4]);
                    float score = record[4];
                    if (score > ob_det_thresh)
                    {
                        cx = ((record[0]) + (float)shift_x) * (float)stride;
                        cy = ((record[1]) + (float)shift_y) * (float)stride;
                        w = exp((record[2])) * (float)stride;
                        h = exp((record[3])) * (float)stride;
                        cx -= ((kmodel_frame_size.width - frame_size.width * gain) / 2);
                        cy -= ((kmodel_frame_size.height - frame_size.height * gain) / 2);
                        cx /= gain;
                        cy /= gain;
                        w /= gain;
                        h /= gain;
                        ob_det_res box;
                        box.x1 = std::max(0, std::min(int(frame_size.width), int(cx - w / 2.f)));
                        box.y1 = std::max(0, std::min(int(frame_size.height), int(cy - h / 2.f)));
                        box.x2 = std::max(0, std::min(int(frame_size.width), int(cx + w / 2.f)));
                        box.y2 = std::max(0, std::min(int(frame_size.height), int(cy + h / 2.f)));
                        box.score = score;
                        box.label_index = cls;
                        result.push_back(box);
                    }
                }
            }
        }
    }
    return result;
}

vector<vector<ob_det_res>> anchorfreedet_decode_infer_class(float* data, FrameSize kmodel_frame_size, FrameSize frame_size, int stride, int num_class, float ob_det_thresh)
{
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    std::vector<std::vector<ob_det_res>> result;
    for (int i = 0; i < num_class; i++)
    {
        result.push_back(vector<ob_det_res>());//不断往v2d里加行 
    }
    int grid_size_w = kmodel_frame_size.width / stride;
    int grid_size_h = kmodel_frame_size.height / stride;
    int one_rsize = num_class + 5;
    float cx, cy, w, h;
    for (int shift_y = 0; shift_y < grid_size_h; shift_y++)
    {
        for (int shift_x = 0; shift_x < grid_size_w; shift_x++)
        {

            int loc = shift_x + shift_y * grid_size_w;
            for (int i = 0; i < 1; i++)
            {
                float* record = data + (loc + i) * one_rsize;
                float* cls_ptr = record + 5;
                for (int cls = 0; cls < num_class; cls++)
                {
                    // float score = sigmoid(record[4]);
                    float score = record[4];
                    if (score > ob_det_thresh)
                    {
                        cx = ((record[0]) + (float)shift_x) * (float)stride;
                        cy = ((record[1]) + (float)shift_y) * (float)stride;
                        w = exp((record[2])) * (float)stride;
                        h = exp((record[3])) * (float)stride;
                        cx -= ((kmodel_frame_size.width - frame_size.width * gain) / 2);
                        cy -= ((kmodel_frame_size.height - frame_size.height * gain) / 2);
                        cx /= gain;
                        cy /= gain;
                        w /= gain;
                        h /= gain;
                        ob_det_res box;
                        box.x1 = std::max(0, std::min(int(frame_size.width), int(cx - w / 2.f)));
                        box.y1 = std::max(0, std::min(int(frame_size.height), int(cy - h / 2.f)));
                        box.x2 = std::max(0, std::min(int(frame_size.width), int(cx + w / 2.f)));
                        box.y2 = std::max(0, std::min(int(frame_size.height), int(cy + h / 2.f)));
                        box.score = score;
                        box.label_index = cls;
                        result[cls].push_back(box);
                    }
                }
            }
        }
    }
    return result;
}

void nms(vector<ob_det_res>& input_boxes, float ob_nms_thresh)
{
    std::sort(input_boxes.begin(), input_boxes.end(), [](ob_det_res a, ob_det_res b) { return a.score > b.score; });
    std::vector<float> vArea(input_boxes.size());
    for (int i = 0; i < int(input_boxes.size()); ++i)
    {
        vArea[i] = (input_boxes.at(i).x2 - input_boxes.at(i).x1 + 1)
            * (input_boxes.at(i).y2 - input_boxes.at(i).y1 + 1);
    }
    for (int i = 0; i < int(input_boxes.size()); ++i)
    {
        for (int j = i + 1; j < int(input_boxes.size());)
        {
            float xx1 = std::max(input_boxes[i].x1, input_boxes[j].x1);
            float yy1 = std::max(input_boxes[i].y1, input_boxes[j].y1);
            float xx2 = std::min(input_boxes[i].x2, input_boxes[j].x2);
            float yy2 = std::min(input_boxes[i].y2, input_boxes[j].y2);
            float w = std::max(float(0), xx2 - xx1 + 1);
            float h = std::max(float(0), yy2 - yy1 + 1);
            float inter = w * h;
            float ovr = inter / (vArea[i] + vArea[j] - inter);
            if (ovr >= ob_nms_thresh)
            {
                input_boxes.erase(input_boxes.begin() + j);
                vArea.erase(vArea.begin() + j);
            }
            else
            {
                j++;
            }
        }
    }
}

float fast_exp(float x)
{
    union {
        uint32_t i;
        float f;
    } v{};
    v.i = (1 << 23) * (1.4426950409 * x + 126.93490512f);
    return v.f;
}

float sigmoid(float x)
{
    return 1.0f / (1.0f + fast_exp(-x));
    //return 1.0f / (1.0f + exp(-x));
}

template<typename _Tp>
int activation_function_softmax(const _Tp* src, _Tp* dst, int length)
{
    const _Tp alpha = *std::max_element(src, src + length);
    _Tp denominator{ 0 };

    for (int i = 0; i < length; ++i)
    {
        dst[i] = fast_exp(src[i] - alpha);
        //dst[i] = exp(src[i] - alpha);
        denominator += dst[i];
    }

    for (int i = 0; i < length; ++i)
    {
        dst[i] /= denominator;
    }

    return 0;
}

ob_det_res disPred2Bbox(const float*& dfl_det, int label, float score, int x, int y, int stride, int reg_max, int input_height,int input_width,float ratiow, float ratioh, float gain,FrameSize frame_size, FrameSize kmodel_frame_size)
{
    float ct_x = x * stride;
    float ct_y = y * stride;

    ct_x -= ((kmodel_frame_size.width - frame_size.width * gain) / 2);
    ct_y -= ((kmodel_frame_size.height - frame_size.height * gain) / 2);
    ct_x /= gain;
    ct_y /= gain;

    std::vector<float> dis_pred;
    dis_pred.resize(4);
    for (int i = 0; i < 4; i++)
    {
        float dis = 0;
        float* dis_after_sm = new float[reg_max + 1];
        activation_function_softmax(dfl_det + i * (reg_max + 1), dis_after_sm, reg_max + 1);
        for (int j = 0; j < reg_max + 1; j++)
            dis += j * dis_after_sm[j];
        
        dis *= stride;
        dis_pred[i] = dis;
        delete[] dis_after_sm;
    }
    float xmin = (std::max)(ct_x - dis_pred[0] /gain, .0f);
    float ymin = (std::max)(ct_y - dis_pred[1] /gain, .0f);
    float xmax = (std::min)(ct_x + dis_pred[2] / gain, (float)frame_size.width);
    float ymax = (std::min)(ct_y + dis_pred[3] / gain, (float)frame_size.height);

    return ob_det_res{ xmin, ymin, xmax, ymax, score, label};
}


void gfldet_decode_infer(float* pred, std::vector<CenterPrior>& center_priors,  std::vector<ob_det_res>& results, FrameSize frame_size, FrameSize kmodel_frame_size, int num_class, float ob_det_thresh)
{
    int reg_max = REG_MAX;
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    const int num_points = center_priors.size();
    const int num_channels = num_class + (reg_max + 1) * 4;
    for (int idx = 0; idx < num_points; idx++)
    {
        int ct_x = center_priors[idx].x;
        int ct_y = center_priors[idx].y;
        int stride = center_priors[idx].stride;
        float score = 0;
        int cur_label = 0;

        for (int label = 0; label < num_class; label++)
        {
            
            float sig_score = sigmoid(pred[idx * num_channels + label]);
            if (sig_score > score)
            {
                score = sig_score;
                cur_label = label;
            }
        }

        if (score > ob_det_thresh)
        {
            const float* bbox_pred = pred + idx * num_channels + num_class;
            results.push_back(disPred2Bbox(bbox_pred, cur_label, score, ct_x, ct_y, stride, reg_max, kmodel_frame_size.height, kmodel_frame_size.width, ratiow, ratioh, gain, frame_size, kmodel_frame_size));
        }
    }
}

void gfldet_decode_infer_class(float* pred, std::vector<CenterPrior>& center_priors,  std::vector<std::vector<ob_det_res>>& results, FrameSize frame_size, FrameSize kmodel_frame_size, int num_class, float ob_det_thresh)
{
    int reg_max = REG_MAX;
    float ratiow = (float)kmodel_frame_size.width / frame_size.width;
    float ratioh = (float)kmodel_frame_size.height / frame_size.height;
    float gain = ratiow < ratioh ? ratiow : ratioh;
    const int num_points = center_priors.size();
    const int num_channels = num_class + (reg_max + 1) * 4;
    for (int idx = 0; idx < num_points; idx++)
    {
        int ct_x = center_priors[idx].x;
        int ct_y = center_priors[idx].y;
        int stride = center_priors[idx].stride;
        float score = 0;
        int cur_label = 0;

        for (int label = 0; label < num_class; label++)
        {
            
            float sig_score = sigmoid(pred[idx * num_channels + label]);
            if (sig_score > score)
            {
                score = sig_score;
                cur_label = label;
            }
        }

        if (score > ob_det_thresh)
        {
            const float* bbox_pred = pred + idx * num_channels + num_class;
            ob_det_res tmp_res = disPred2Bbox(bbox_pred, cur_label, score, ct_x, ct_y, stride, reg_max, kmodel_frame_size.height, kmodel_frame_size.width, ratiow, ratioh, gain, frame_size, kmodel_frame_size);
            results[tmp_res.label_index].push_back(tmp_res);
        }
    }
}



ob_det_res* anchorbasedet_post_process(float* data0, float* data1, float* data2, FrameSize kmodel_frame_size, FrameSize frame_size, int* strides, int num_class, float ob_det_thresh, float ob_nms_thresh, float* anchors, bool nms_option, int* results_size)
{
    float *output_0 = data0;
    float *output_1 = data1;
    float *output_2 = data2;

    float anchors_0[3][2];
    float anchors_1[3][2];
    float anchors_2[3][2];
    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            anchors_0[i][j] = anchors[i*2+j];
        }

    }

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            anchors_1[i][j] = anchors[1*2*3 + i*2 + j];
        }

    }

    for(int i = 0; i < 3; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            anchors_2[i][j] = anchors[2*2*3 + i*2 + j];
        }

    }


    vector<ob_det_res> results;
    if (nms_option)
    {
        vector<ob_det_res> box0, box1, box2;
        box0 = anchorbasedet_decode_infer(output_0, kmodel_frame_size, frame_size, strides[0], num_class, ob_det_thresh, anchors_0);
        box1 = anchorbasedet_decode_infer(output_1, kmodel_frame_size, frame_size, strides[1], num_class, ob_det_thresh, anchors_1);
        box2 = anchorbasedet_decode_infer(output_2, kmodel_frame_size, frame_size, strides[2], num_class, ob_det_thresh, anchors_2);

        results.insert(results.begin(), box0.begin(), box0.end());
        results.insert(results.begin(), box1.begin(), box1.end());
        results.insert(results.begin(), box2.begin(), box2.end());
        
        nms(results, ob_nms_thresh);
    }
    else
    {
        vector<vector<ob_det_res>> box0, box1, box2;

        box0 = anchorbasedet_decode_infer_class(output_0, kmodel_frame_size, frame_size, strides[0], num_class, ob_det_thresh, anchors_0);
        box1 = anchorbasedet_decode_infer_class(output_1, kmodel_frame_size, frame_size, strides[1], num_class, ob_det_thresh, anchors_1);
        box2 = anchorbasedet_decode_infer_class(output_2, kmodel_frame_size, frame_size, strides[2], num_class, ob_det_thresh, anchors_2);

        for(int i = 0; i < num_class; i++)
        {
            box0[i].insert(box0[i].begin(), box1[i].begin(), box1[i].end());
            box0[i].insert(box0[i].begin(), box2[i].begin(), box2[i].end());
            nms(box0[i], ob_nms_thresh);
            results.insert(results.begin(), box0[i].begin(), box0[i].end());
        }
    }

    *results_size = results.size();
    ob_det_res* results_ob = (ob_det_res *)malloc(*results_size * sizeof(ob_det_res));
    for (int i = 0; i < *results_size; i++) {
        results_ob[i] = results[i];
    }
    return results_ob;
}


ob_det_res* anchorfreedet_post_process(float* data0, float* data1, float* data2, FrameSize kmodel_frame_size, FrameSize frame_size, int* strides, int num_class, float ob_det_thresh, float ob_nms_thresh, bool nms_option, int* results_size)
{
    float *output_0 = data0;
    float *output_1 = data1;
    float *output_2 = data2;


    vector<ob_det_res> results;
    if (nms_option)
    {
        vector<ob_det_res> box0, box1, box2;

        box0 = anchorfreedet_decode_infer(output_0, kmodel_frame_size, frame_size, strides[0], num_class, ob_det_thresh);
        box1 = anchorfreedet_decode_infer(output_1, kmodel_frame_size, frame_size, strides[1], num_class, ob_det_thresh);
        box2 = anchorfreedet_decode_infer(output_2, kmodel_frame_size, frame_size, strides[2], num_class, ob_det_thresh);

        results.insert(results.begin(), box0.begin(), box0.end());
        results.insert(results.begin(), box1.begin(), box1.end());
        results.insert(results.begin(), box2.begin(), box2.end());
        
        nms(results, ob_nms_thresh);
    }
    else
    {
        vector<vector<ob_det_res>> box0, box1, box2;

        box0 = anchorfreedet_decode_infer_class(output_0, kmodel_frame_size, frame_size, strides[0], num_class, ob_det_thresh);
        box1 = anchorfreedet_decode_infer_class(output_1, kmodel_frame_size, frame_size, strides[1], num_class, ob_det_thresh);
        box2 = anchorfreedet_decode_infer_class(output_2, kmodel_frame_size, frame_size, strides[2], num_class, ob_det_thresh);

        for(int i = 0; i < num_class; i++)
        {
            box0[i].insert(box0[i].begin(), box1[i].begin(), box1[i].end());
            box0[i].insert(box0[i].begin(), box2[i].begin(), box2[i].end());
            nms(box0[i], ob_nms_thresh);
            results.insert(results.begin(), box0[i].begin(), box0[i].end());
        }
    }

    *results_size = results.size();
    ob_det_res* results_ob = (ob_det_res *)malloc(*results_size * sizeof(ob_det_res));
    for (int i = 0; i < *results_size; i++) {
        results_ob[i] = results[i];
    }
    return results_ob;
}


ob_det_res* gfldet_post_process(float* data0, float* data1, float* data2, FrameSize kmodel_frame_size, FrameSize frame_size, int* strides, int num_class, float ob_det_thresh, float ob_nms_thresh, bool nms_option, int* results_size)
{
    float *output_0 = data0;
    float *output_1 = data1;
    float *output_2 = data2;

    vector<CenterPrior> center_priors[3];
    for (int i = 0; i < STAGE_NUM; i++)
    {
        int stride = strides[i];
        int feat_w = ceil((float)kmodel_frame_size.width / stride);
        int feat_h = ceil((float)kmodel_frame_size.height / stride);
        for (int y = 0; y < feat_h; y++)
            for (int x = 0; x < feat_w; x++)
            {
                CenterPrior ct;
                ct.x = x;
                ct.y = y;
                ct.stride = stride;
                center_priors[i].push_back(ct);
            }
        
    }

    vector<ob_det_res> results;
    if(nms_option)
    {
        vector<ob_det_res> b0, b1, b2;

        gfldet_decode_infer(output_0, center_priors[0], b0, frame_size, kmodel_frame_size, num_class, ob_det_thresh);
        gfldet_decode_infer(output_1, center_priors[1], b1, frame_size, kmodel_frame_size, num_class, ob_det_thresh);
        gfldet_decode_infer(output_2, center_priors[2], b2, frame_size, kmodel_frame_size, num_class, ob_det_thresh);

        results.insert(results.begin(), b0.begin(), b0.end());
        results.insert(results.begin(), b1.begin(), b1.end());
        results.insert(results.begin(), b2.begin(), b2.end());

        nms(results, ob_nms_thresh);
    }
    else
    {
        vector<vector<ob_det_res>> b0, b1, b2;
        for (int i = 0; i < num_class; i++)
        {
            b0.push_back(vector<ob_det_res>());//不断往v2d里加行 
            b1.push_back(vector<ob_det_res>());//不断往v2d里加行 
            b2.push_back(vector<ob_det_res>());//不断往v2d里加行 
        }

        gfldet_decode_infer_class(output_0, center_priors[0], b0, frame_size, kmodel_frame_size, num_class, ob_det_thresh);
        gfldet_decode_infer_class(output_1, center_priors[1], b1, frame_size, kmodel_frame_size, num_class, ob_det_thresh);
        gfldet_decode_infer_class(output_2, center_priors[2], b2, frame_size, kmodel_frame_size, num_class, ob_det_thresh);

        for(int i = 0; i < num_class; i++)
        {
            b0[i].insert(b0[i].begin(), b1[i].begin(), b1[i].end());
            b0[i].insert(b0[i].begin(), b2[i].begin(), b2[i].end());
            nms(b0[i], ob_nms_thresh);
            results.insert(results.begin(), b0[i].begin(), b0[i].end());
        }
    }
    *results_size = results.size();
    ob_det_res* results_ob = (ob_det_res *)malloc(*results_size * sizeof(ob_det_res));
    for (int i = 0; i < *results_size; i++) {
        results_ob[i] = results[i];
    }
    return results_ob;
}

uint8_t* seg_post_process(float* data, int num_class, FrameSize ori_shape, FrameSize dst_shape)
{
    float* output = data;
    cv::Mat images_pred_color = cv::Mat::zeros(ori_shape.height, ori_shape.width, CV_8UC4);

    for (int y = 0; y < ori_shape.height; ++y)
    {
        for (int x = 0; x < ori_shape.width; ++x)
        {
            float s = 0.0;
            int loc = num_class * (x + y * ori_shape.width);
            for (int c = 0; c < num_class; c++)
                s += exp(output[loc + c]);
            vector<float> scores(num_class);
            
            scores.clear();
            for (int c = 0; c < num_class; c++)
            {
                output[loc + c] = output[loc + c] / s;
                scores.push_back(output[loc + c]);
            }
            cv::Vec4b& color = images_pred_color.at<cv::Vec4b>(cv::Point(x, y));
            color[0] = 0;
            color[1] = 0;
            color[2] = 0;
            color[3] = 128;
            float score0 = scores[0];
            for (int i = 1; i < num_class; ++i)
            {
                if (scores[i] > score0)
                {
                    score0 = scores[i];
                    color[0] = max(255 - i * 60, 0);
                    color[1] = min(i * 80, 255);
                    color[2] = max(255 - (num_class - i) * 100, 255);
                }
            }
        }
    }
    cv::resize(images_pred_color, images_pred_color, cv::Size(dst_shape.width, dst_shape.height));

    uint8_t *result = (uint8_t *)malloc(dst_shape.width * dst_shape.height * 4 * sizeof(uint8_t));
    memcpy(result, images_pred_color.data, sizeof(uint8_t) * dst_shape.width * dst_shape.height * 4);
    return result;
}