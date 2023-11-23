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
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "aidemo_wrap.h"

#include <stdlib.h>
#include <iostream>
#include <unistd.h>

#define SEGCHANNELS 32
#define CLASSES_COUNT 80

typedef struct {
	cv::Rect box;
	float confidence;
	int index;
}BBOX;

struct OutputSeg {
	int id;             //结果类别id
	float confidence;   //结果置信度
	cv::Rect box;       //矩形框
	cv::Mat boxMask;       //矩形框内mask，节省内存空间和加快速度
};

const std::vector<cv::Scalar> color_four = {cv::Scalar(60, 20, 220, 127),
		cv::Scalar(32, 11, 119, 127),
		cv::Scalar(142, 0, 0, 127),
		cv::Scalar(230, 0, 0, 127),
		cv::Scalar(228, 0, 106, 127),
		cv::Scalar(100, 60, 0, 127),
		cv::Scalar(100, 80, 0, 127),
		cv::Scalar(70, 0, 0, 127),
		cv::Scalar(192, 0, 0, 127),
		cv::Scalar(30, 170, 250, 127),
		cv::Scalar(30, 170, 100, 127),
		cv::Scalar(0, 220, 220, 127),
		cv::Scalar(175, 116, 175, 127),
		cv::Scalar(30, 0, 250, 127),
		cv::Scalar(42, 42, 165, 127),
		cv::Scalar(255, 77, 255, 127),
		cv::Scalar(252, 226, 0, 127),
		cv::Scalar(255, 182, 182, 127),
		cv::Scalar(0, 82, 0, 127),
		cv::Scalar(157, 166, 120, 127),
		cv::Scalar(0, 76, 110, 127),
		cv::Scalar(255, 57, 174, 127),
		cv::Scalar(0, 100, 199, 127),
		cv::Scalar(118, 0, 72, 127),
		cv::Scalar(240, 179, 255, 127),
		cv::Scalar(92, 125, 0, 127),
		cv::Scalar(151, 0, 209, 127),
		cv::Scalar(182, 208, 188, 127),
		cv::Scalar(176, 220, 0, 127),
		cv::Scalar(164, 99, 255, 127),
		cv::Scalar(73, 0, 92, 127),
		cv::Scalar(255, 129, 133, 127),
		cv::Scalar(255, 180, 78, 127),
		cv::Scalar(0, 228, 0, 127),
		cv::Scalar(243, 255, 174, 127),
		cv::Scalar(255, 89, 45, 127),
		cv::Scalar(103, 134, 134, 127),
		cv::Scalar(174, 148, 145, 127),
		cv::Scalar(186, 208, 255, 127),
		cv::Scalar(255, 226, 197, 127),
		cv::Scalar(1, 134, 171, 127),
		cv::Scalar(54, 63, 109, 127),
		cv::Scalar(255, 138, 207, 127),
		cv::Scalar(95, 0, 151, 127),
		cv::Scalar(61, 80, 9, 127),
		cv::Scalar(51, 105, 84, 127),
		cv::Scalar(105, 65, 74, 127),
		cv::Scalar(102, 196, 166, 127),
		cv::Scalar(210, 195, 208, 127),
		cv::Scalar(65, 109, 255, 127),
		cv::Scalar(149, 143, 0, 127),
		cv::Scalar(194, 0, 179, 127),
		cv::Scalar(106, 99, 209, 127),
		cv::Scalar(0, 121, 5, 127),
		cv::Scalar(205, 255, 227, 127),
		cv::Scalar(208, 186, 147, 127),
		cv::Scalar(1, 69, 153, 127),
		cv::Scalar(161, 95, 3, 127),
		cv::Scalar(0, 255, 163, 127),
		cv::Scalar(170, 0, 119, 127),
		cv::Scalar(199, 182, 0, 127),
		cv::Scalar(120, 165, 0, 127),
		cv::Scalar(88, 130, 183, 127),
		cv::Scalar(0, 32, 95, 127),
		cv::Scalar(135, 114, 130, 127),
		cv::Scalar(133, 129, 110, 127),
		cv::Scalar(118, 74, 166, 127),
		cv::Scalar(185, 142, 219, 127),
		cv::Scalar(114, 210, 79, 127),
		cv::Scalar(62, 90, 178, 127),
		cv::Scalar(15, 70, 65, 127),
		cv::Scalar(115, 167, 127, 127),
		cv::Scalar(106, 105, 59, 127),
		cv::Scalar(45, 108, 142, 127),
		cv::Scalar(0, 172, 196, 127),
		cv::Scalar(80, 54, 95, 127),
		cv::Scalar(255, 76, 128, 127),
		cv::Scalar(1, 57, 201, 127),
		cv::Scalar(122, 0, 246, 127),
		cv::Scalar(208, 162, 191, 127)};

float get_iou_value(cv::Rect rect1, cv::Rect rect2)
{
	int xx1, yy1, xx2, yy2;
 
	xx1 = std::max(rect1.x, rect2.x);
	yy1 = std::max(rect1.y, rect2.y);
	xx2 = std::min(rect1.x + rect1.width - 1, rect2.x + rect2.width - 1);
	yy2 = std::min(rect1.y + rect1.height - 1, rect2.y + rect2.height - 1);
 
	int insection_width, insection_height;
	insection_width = std::max(0, xx2 - xx1 + 1);
	insection_height = std::max(0, yy2 - yy1 + 1);
 
	float insection_area, union_area, iou;
	insection_area = float(insection_width) * insection_height;
	union_area = float(rect1.width*rect1.height + rect2.width*rect2.height - insection_area);
	iou = insection_area / union_area;

	return iou;
}

void nms_boxes(std::vector<cv::Rect> &boxes, std::vector<float> &confidences, float confThreshold, float nmsThreshold, std::vector<int> &indices)
{	
	BBOX bbox;
	std::vector<BBOX> bboxes;
	int i, j;
	for (i = 0; i < boxes.size(); i++)
	{
		bbox.box = boxes[i];
		bbox.confidence = confidences[i];
		bbox.index = i;
		bboxes.push_back(bbox);
	}

	sort(bboxes.begin(), bboxes.end(), [](BBOX a, BBOX b) { return a.confidence < b.confidence; });

	int updated_size = bboxes.size();
	for (i = 0; i < updated_size; i++)
	{
		if (bboxes[i].confidence < confThreshold)
			continue;
		indices.push_back(bboxes[i].index);

		for (j = i + 1; j < updated_size;)
		{
			float iou = get_iou_value(bboxes[i].box, bboxes[j].box);

			if (iou > nmsThreshold)
			{
				bboxes.erase(bboxes.begin() + j);
				updated_size = bboxes.size();
			}
            else
            {
                j++;    
            }
		}
	}
}



void draw_segmentation(cv::Mat& frame,std::vector<OutputSeg>& results)
{

	for (int i = 0; i < results.size(); i++) {
		int left, top;
		left = results[i].box.x;
		top = results[i].box.y;
		int color_num = i;
		rectangle(frame, results[i].box, color_four[results[i].id], 2, 8);
		
		frame(results[i].box).setTo(color_four[results[i].id], results[i].boxMask);
	}
}


SegOutputs object_seg_post_process(float *data_0, float *data_1, FrameSize frame_size, FrameSize kmodel_frame_size, FrameSize display_frame_size, float conf_thres, float nms_thres, float mask_thres, int *box_cnt)
{
	std::vector<OutputSeg> results;
    float *output_0 = data_0;
    float *output_1 = data_1;

    int w, h, x, y;
	float r_w = kmodel_frame_size.width / (frame_size.width*1.0);
	float r_h = kmodel_frame_size.height / (frame_size.height*1.0);
	if (r_h > r_w) {
		w = kmodel_frame_size.width;
		h = r_w * frame_size.height;
		x = 0;
		y = (kmodel_frame_size.height - h) / 2;
	}
	else {
		w = r_h * frame_size.width;
		h = kmodel_frame_size.height;
		x = (kmodel_frame_size.width - w) / 2;
		y = 0;
	}

    int newh = h, neww = w, padh = y, padw = x;
	float ratio_h = (float)frame_size.height / newh;
	float ratio_w = (float)frame_size.width / neww;

    std::vector<int> classIds;//结果id数组
	std::vector<float> confidences;//结果每个id对应置信度数组
	std::vector<cv::Rect> boxes;//每个id矩形框
	std::vector<cv::Mat> picked_proposals;  //后续计算mask


	// 处理box
	int Num_box = (kmodel_frame_size.width/8) * (kmodel_frame_size.height/8) + (kmodel_frame_size.width/16) * (kmodel_frame_size.height/16) + (kmodel_frame_size.width/32) * (kmodel_frame_size.height/32);
	int net_length = CLASSES_COUNT + 4 + SEGCHANNELS;
	cv::Mat out1 = cv::Mat(net_length, Num_box, CV_32F, output_0);

	for (int i = 0; i < Num_box; i++) {
		//输出是1*net_length*Num_box;所以每个box的属性是每隔Num_box取一个值，共net_length个值
		cv::Mat scores = out1(cv::Rect(i, 4, 1, CLASSES_COUNT)).clone();
		cv::Point classIdPoint;
		double max_class_socre;
		minMaxLoc(scores, 0, &max_class_socre, 0, &classIdPoint);
		max_class_socre = (float)max_class_socre;
		if (max_class_socre >= conf_thres) {
			cv::Mat temp_proto = out1(cv::Rect(i, 4 + CLASSES_COUNT, 1, SEGCHANNELS)).clone();
			picked_proposals.push_back(temp_proto.t());
			float x = (out1.at<float>(0, i) - padw) * ratio_w * display_frame_size.width / frame_size.width;  //cx
			float y = (out1.at<float>(1, i) - padh) * ratio_h * display_frame_size.height / frame_size.height;  //cy
			float w = out1.at<float>(2, i) * ratio_w * display_frame_size.width / frame_size.width;  //w
			float h = out1.at<float>(3, i) * ratio_h * display_frame_size.height / frame_size.height;  //h
			int left = MAX((x - 0.5 * w), 0);
			int top = MAX((y - 0.5 * h), 0);
			int width = (int)w;
			int height = (int)h;
			if (width <= 0 || height <= 0) { continue; }

			classIds.push_back(classIdPoint.y);
			confidences.push_back(max_class_socre);
			boxes.push_back(cv::Rect(left, top, width, height));
		}

	}

	//执行非最大抑制以消除具有较低置信度的冗余重叠框（NMS）
	std::vector<int> nms_result;
	nms_boxes(boxes, confidences, conf_thres, nms_thres, nms_result);

	std::vector<cv::Mat> temp_mask_proposals;
	std::vector<OutputSeg> output;
	cv::Rect holeImgRect(0, 0, display_frame_size.width, display_frame_size.height);
	for (int i = 0; i < nms_result.size(); ++i) {
		int idx = nms_result[i];
		OutputSeg result;
		result.id = classIds[idx];
		result.confidence = confidences[idx];
		result.box = boxes[idx]& holeImgRect;
		output.push_back(result);
		temp_mask_proposals.push_back(picked_proposals[idx]);
	}

	// 处理mask
	int segWidth = kmodel_frame_size.width/4;
    int segHeight = kmodel_frame_size.height/4;
	if(temp_mask_proposals.size() > 0)
	{	cv::Mat maskProposals;
		for (int i = 0; i < temp_mask_proposals.size(); ++i)
		{
			maskProposals.push_back(temp_mask_proposals[i]);
		}

		cv::Mat protos = cv::Mat(SEGCHANNELS, segWidth * segHeight, CV_32FC1, output_1);
		sync();
		cv::Mat matmulRes = (maskProposals * protos).t();//n*32 32*25600 A*B是以数学运算中矩阵相乘的方式实现的，要求A的列数等于B的行数时
		cv::Mat masks = matmulRes.reshape(output.size(), { segWidth,segHeight });//n*160*160

		std::vector<cv::Mat> maskChannels;
		cv::split(masks, maskChannels);
		cv::Rect roi(int((float)padw / kmodel_frame_size.width * segWidth), int((float)padh / kmodel_frame_size.height * segHeight), int(segWidth - padw / 2), int(segHeight - padh / 2));

		for (int i = 0; i < output.size(); ++i) {
			cv::Mat dest, mask;
			cv::exp(-maskChannels[i], dest);//sigmoid
			dest = 1.0 / (1.0 + dest);//160*160
			dest = dest(roi);
			resize(dest, mask, cv::Size(display_frame_size.width, display_frame_size.height), cv::INTER_NEAREST);
			//crop----截取box中的mask作为该box对应的mask
			cv::Rect temp_rect = output[i].box;
			mask = mask(temp_rect) > mask_thres;
			output[i].boxMask = mask;
		}
		results=output;
	}
	cv::Mat osd_frame(display_frame_size.height, display_frame_size.width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
	draw_segmentation(osd_frame, results);


	SegOutputs segOutputs;
	segOutputs.masks_results = (uint8_t *)malloc(display_frame_size.width * display_frame_size.height * 4 * sizeof(uint8_t));
    memcpy(segOutputs.masks_results, osd_frame.data, sizeof(uint8_t) * display_frame_size.width * display_frame_size.height * 4 );
	*box_cnt = results.size();
	segOutputs.segOutput = (SegOutput *)malloc(*box_cnt * sizeof(SegOutput));
	for (int i = 0; i < *box_cnt; i++)
	{
		segOutputs.segOutput[i].confidence = results[i].confidence;
		segOutputs.segOutput[i].id = results[i].id;
		segOutputs.segOutput[i].box[0] = results[i].box.x;
		segOutputs.segOutput[i].box[1] = results[i].box.y;
		segOutputs.segOutput[i].box[2] = results[i].box.width;
		segOutputs.segOutput[i].box[3] = results[i].box.height;
	}

	return segOutputs;
}