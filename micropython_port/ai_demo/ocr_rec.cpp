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
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "aidemo_wrap.h"

typedef struct BoxPoint
{
    cv::Point2f vertices[4];
}BoxPoint;

float* mask_resize(float* dest, FrameSize ori_shape, FrameSize tag_shape)
{
    cv::Mat dest_mat(ori_shape.height, ori_shape.width, CV_32FC1, dest);
    cv::Mat mask;
    resize(dest_mat, mask, cv::Size(tag_shape.width, tag_shape.height), cv::INTER_NEAREST);
    float *result = (float *)malloc(tag_shape.width * tag_shape.height * sizeof(float));
    memcpy(result, mask.data, sizeof(float) * tag_shape.width * tag_shape.height);
    return result;
}

std::vector<size_t> sort_indices_(const std::vector<cv::Point2f>& vec) 
{
	std::vector<std::pair<cv::Point2f, size_t>> indexedVec;
	indexedVec.reserve(vec.size());

	// 创建带有索引的副本
	for (size_t i = 0; i < vec.size(); ++i) {
		indexedVec.emplace_back(vec[i], i);
	}

	// 按值对副本进行排序
	std::sort(indexedVec.begin(), indexedVec.end(),
		[](const auto& a, const auto& b) {
		return a.first.x < b.first.x;
	});

	// 提取排序后的索引
	std::vector<size_t> sortedIndices;
	sortedIndices.reserve(vec.size());
	for (const auto& element : indexedVec) {
		sortedIndices.push_back(element.second);
	}

	return sortedIndices;
}

void find_rectangle_vertices_(const std::vector<cv::Point2f>& points, cv::Point2f& topLeft, cv::Point2f& topRight, cv::Point2f& bottomRight, cv::Point2f& bottomLeft) 
{
    //先按照x排序,比较左右，再按照y比较上下
	auto sorted_x_id = sort_indices_(points);

	if (points[sorted_x_id[0]].y < points[sorted_x_id[1]].y)
	{
		topLeft = points[sorted_x_id[0]];
		bottomLeft = points[sorted_x_id[1]];
	}
	else
	{
		topLeft = points[sorted_x_id[1]];
		bottomLeft = points[sorted_x_id[0]];
	}

	if (points[sorted_x_id[2]].y < points[sorted_x_id[3]].y)
	{
        bottomRight = points[sorted_x_id[3]];
		topRight = points[sorted_x_id[2]];

	}
	else
	{ 
        bottomRight = points[sorted_x_id[2]];
		topRight = points[sorted_x_id[3]];
	}
	
}

void warppersp_(cv::Mat src, cv::Mat& dst, BoxPoint b, std::vector<cv::Point2f>& vtd)
{
    cv::Mat rotation;
    std::vector<cv::Point> con;
    for(auto i : b.vertices)
        con.push_back(i);

    cv::RotatedRect minrect = minAreaRect(con);
    std::vector<cv::Point2f> vtx(4),vt(4);
    minrect.points(vtx.data());

    find_rectangle_vertices_(vtx, vtd[0], vtd[1], vtd[2], vtd[3]);
    
    //w,h tmp_w=dist(p1,p0),tmp_h=dist(p1,p2)
    float tmp_w = cv::norm(vtd[1]-vtd[0]);
    float tmp_h = cv::norm(vtd[2]-vtd[1]);
    float w = std::max(tmp_w,tmp_h);
    float h = std::min(tmp_w,tmp_h);

    vt[0].x = 0;
    vt[0].y = 0;
    vt[1].x = w;//w
    vt[1].y = 0;
    vt[2].x = w;
    vt[2].y = h;
    vt[3].x = 0;
    vt[3].y = h;//h
    rotation = cv::getPerspectiveTransform(vtd, vt);

    cv::warpPerspective(src, dst, rotation, cv::Size(w, h));
}

ArrayWrapperMat1* ocr_rec_pre_process(uint8_t* data, FrameSize ori_shape, BoxPoint8* boxpoint8, int box_cnt)
{
    int matsize = ori_shape.width * ori_shape.height;
    cv::Mat ori_img;
    cv::Mat ori_img_R = cv::Mat(ori_shape.height, ori_shape.width, CV_8UC1, data);
    cv::Mat ori_img_G = cv::Mat(ori_shape.height, ori_shape.width, CV_8UC1, data + 1 * matsize);
    cv::Mat ori_img_B = cv::Mat(ori_shape.height, ori_shape.width, CV_8UC1, data + 2 * matsize);
    std::vector<cv::Mat> sensor_bgr;
    sensor_bgr.push_back(ori_img_B);
    sensor_bgr.push_back(ori_img_G);
    sensor_bgr.push_back(ori_img_R);
    cv::merge(sensor_bgr, ori_img);
    std::vector<BoxPoint> results_det;
    results_det.clear();
    for(int i = 0; i < box_cnt; i++)
    {
        BoxPoint boxpoint;
        for(int j = 0; j < 4; j++)
        {
            boxpoint.vertices[j].x = boxpoint8[i].points8[2 * j + 0];
            boxpoint.vertices[j].y = boxpoint8[i].points8[2 * j + 1];
        }
        results_det.push_back(boxpoint);
    }

    ArrayWrapperMat1 *arrayWrapperMat1 = (ArrayWrapperMat1 *)malloc(box_cnt * sizeof(ArrayWrapperMat1));

    for(int i = 0; i < results_det.size(); i++)
    {
        std::vector<cv::Point2f> sort_vtd(4);
        cv::Mat crop;
        warppersp_(ori_img, crop, results_det[i], sort_vtd);
        cv::Mat crop_gray;
        cv::cvtColor(crop, crop_gray, cv::COLOR_BGR2GRAY);

        arrayWrapperMat1[i].data = (uint8_t *)malloc(crop_gray.cols * crop_gray.rows * sizeof(uint8_t));
        memcpy(arrayWrapperMat1[i].data, crop_gray.data, crop_gray.cols * crop_gray.rows * sizeof(uint8_t));
        arrayWrapperMat1[i].framesize.width = crop_gray.cols;
        arrayWrapperMat1[i].framesize.height = crop_gray.rows;
        for(int j = 0; j < 4; j++)
        {
            arrayWrapperMat1[i].coordinates[2 * j + 0] = sort_vtd[j].x;
            arrayWrapperMat1[i].coordinates[2 * j + 1] = sort_vtd[j].y;
        }
    }
    return arrayWrapperMat1;
}