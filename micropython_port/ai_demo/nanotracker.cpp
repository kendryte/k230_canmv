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
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.f
 */

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "aidemo_wrap.h"

#include <stdlib.h>
#include <iostream>

#define OUTPUT_1_SIZE 2  // 两个输出，与points变量联系
#define WINDOW_INFLUENCE 0.46  // 框作用范围系数
#define LR  0.34  // rect_size 调整系数
#define OUTPUT_GRID 16  
#define OUTPUT_GRID_SIZE 256 
#define PENALTY_K 0.16 // pscore调整系数

float center[2]; // 中心坐标
float rect_size[2]; // 初始化 rect_size
float window[OUTPUT_GRID_SIZE];
float points[OUTPUT_GRID_SIZE][OUTPUT_1_SIZE];
float hhanning[16] = { 0., 0.04322727, 0.1654347, 0.3454915, 0.55226423, 0.75, 0.9045085, 0.9890738,
		0.9890738 , 0.9045085, 0.75, 0.55226423, 0.3454915, 0.1654347, 0.04322727, 0. }; // 汉宁窗取值

void set_han()
{
	int i, j;
	for (i = 0; i < OUTPUT_GRID; i++)
		for (j = 0; j < OUTPUT_GRID; j++)
			window[i * OUTPUT_GRID + j] = hhanning[i] * hhanning[j];

}

void set_points()
{
	int i, j;
	for (i = 0; i < OUTPUT_GRID; i++)
	{
		float x = -128.0;
		for (j = 0; j < OUTPUT_GRID; j++)
		{
			points[i * OUTPUT_GRID + j][0] = x;
			x += OUTPUT_GRID;
		}	
	}
	float y = -128.0;
	for (i = 0; i < OUTPUT_GRID; i++)
	{
		for (j = 0; j < OUTPUT_GRID; j++)
			points[i * OUTPUT_GRID + j][1] = y;
		y += OUTPUT_GRID;
	}
}


void softmax(float* score)
{
	float sum0 = 0.0, sum1 = 0.0;
	for (int i = 0; i < OUTPUT_GRID_SIZE; i++)
		sum1 += exp(score[OUTPUT_GRID_SIZE + i]);
	for (int i = 0; i < OUTPUT_GRID_SIZE; i++)
		score[i + OUTPUT_GRID_SIZE] = exp(score[OUTPUT_GRID_SIZE + i]) / sum1;
}

float* convert_score(float* score)
{
	softmax(score);
	return score;
}

void corner2center(float& x, float& y, float& w, float& h)
{
	float x1 = x;
	float y1 = y;
	float x2 = w;
	float y2 = h;
	x = (x1 + x2) * 0.5;
	y = (y1 + y2) * 0.5;
	w = x2 - x1;
	h = y2 - y1;
}

float* convert_bbox(float* box)
{
	int i;
	for (i = 0; i < OUTPUT_GRID_SIZE; i++)
	{
		box[i] 			 = points[i][0] - box[i];
		box[OUTPUT_GRID_SIZE + i] 	 = points[i][1] - box[OUTPUT_GRID_SIZE + i];
		box[OUTPUT_GRID_SIZE * 2 + i] = points[i][0] + box[OUTPUT_GRID_SIZE * 2 + i];
		box[OUTPUT_GRID_SIZE * 3 + i] = points[i][1] + box[OUTPUT_GRID_SIZE * 3 + i];
		corner2center(box[i], box[OUTPUT_GRID_SIZE + i], box[OUTPUT_GRID_SIZE * 2 + i], box[OUTPUT_GRID_SIZE * 3 + i]);
	}		

	return box;
}

float change(float r)
{
	return std::max(r, (float)(1.0 / r));
}

float sz(float w, float h)
{
	float pad = (w + h) * 0.5;
	return sqrt((w + pad) * (h * pad));
}

int max_index(float* s)
{
	int index = 0;
	float max = s[0];
	for (int i = 1; i < OUTPUT_GRID_SIZE; i++)
		if (max < s[i])
		{
			max = s[i];
			index = i;
		}
	return index;
}

void bbox_clip(float& x, float& y, float& w, float& h, int cols, int rows)
{
	float cx = x, cy = y, cw = w, ch = h;
	x = std::max((float)0.0,  (float)std::min(cx, (float)(cols * 1.0)));
	y = std::max((float)0.0,  (float)std::min(cy, (float)(rows * 1.0)));
	w = std::max((float)10.0, (float)std::min(cw, (float)(cols * 1.0)));
	h = std::max((float)10.0, (float)std::min(ch, (float)(rows * 1.0)));
}

void track_post_process(float* score, float* box, int cols,int rows, int& box_x, int& box_y, int& box_w, int& box_h, float& best_score, int crop_size, float CONTEXT_AMOUNT)
{
    float s_z = round(sqrt((rect_size[0] + CONTEXT_AMOUNT * (rect_size[0] + rect_size[1])) * (rect_size[1] + CONTEXT_AMOUNT * (rect_size[0] + rect_size[1]))));
	float scale_z = crop_size / s_z;

	score = convert_score(score);
	box = convert_bbox(box);
	float w[OUTPUT_GRID_SIZE];
	float h[OUTPUT_GRID_SIZE];
	float sc[OUTPUT_GRID_SIZE], rc[OUTPUT_GRID_SIZE];
	float penalty[OUTPUT_GRID_SIZE], pscore[OUTPUT_GRID_SIZE];
	for (int i = 0; i < OUTPUT_GRID_SIZE; i++) 
	{
		w[i] = box[OUTPUT_GRID_SIZE * 2 + i];
		h[i] = box[OUTPUT_GRID_SIZE * 3 + i];
	}

	for (int i = 0; i < OUTPUT_GRID_SIZE; i++)
	{
		float tmps = sz(w[i], h[i]) / sz(rect_size[0] * scale_z, rect_size[1] * scale_z);
		sc[i] = change(tmps);
		float tmpr = (rect_size[0] / rect_size[1]) / (w[i] / h[i]);
		rc[i] = change(tmpr);
		penalty[i] = exp(-(rc[i] * sc[i] - 1) * PENALTY_K);
		pscore[i] = penalty[i] * score[OUTPUT_GRID_SIZE + i];
		pscore[i] = pscore[i] * (1 - WINDOW_INFLUENCE) + window[i] * WINDOW_INFLUENCE;

	}
	int best_index = max_index(pscore);
	float cx = box[best_index] / scale_z;
	float cy = box[best_index + OUTPUT_GRID_SIZE] / scale_z;
	float cw = box[best_index + OUTPUT_GRID_SIZE * 2] / scale_z;
	float ch = box[best_index + OUTPUT_GRID_SIZE * 3] / scale_z;
	float lr = penalty[best_index] * score[OUTPUT_GRID_SIZE + best_index] * LR;

	cx = cx + center[0];
	cy = cy + center[1];
	cw = rect_size[0] * (1 - lr) + cw * lr;
	ch = rect_size[1] * (1 - lr) + ch * lr;
	bbox_clip(cx, cy, cw, ch, cols, rows);
	center[0] = cx;
	center[1] = cy;
	rect_size[0] = cw;
	rect_size[1] = ch;
	best_score = score[OUTPUT_GRID_SIZE+best_index];
	box_x = std::max(0, int(cx - cw / 2));
	box_y = std::max(0, int(cy - ch / 2));
	box_w = int(cw);
	box_h = int(ch);
}

Tracker_box_center nanotracker_post_process(float* output_0, float* output_1, FrameSize sensor_size, float thresh, float* center_xy_wh, int crop_size, float CONTEXT_AMOUNT)
{
	center[0] = center_xy_wh[0];
    center[1] = center_xy_wh[1];
    rect_size[0] = center_xy_wh[2];
    rect_size[1] = center_xy_wh[3];

	set_han();
	set_points();

	int cols = sensor_size.width;
	int rows = sensor_size.height;
    int box_x, box_y, box_h, box_w;
    float best_score;
    track_post_process(output_0, output_1, cols, rows, box_x, box_y, box_w, box_h, best_score, crop_size, CONTEXT_AMOUNT);

	Tracker_box_center track_box_center;
	track_box_center.exist = false;
    if (best_score > thresh)
    {
        track_box_center.tracker_box.x = box_x;
        track_box_center.tracker_box.y = box_y;
        track_box_center.tracker_box.w = box_w;
        track_box_center.tracker_box.h = box_h;
        track_box_center.tracker_box.score = best_score;
		track_box_center.exist = true;
    }
	track_box_center.center_xy_wh[0] = center[0];
	track_box_center.center_xy_wh[1] = center[1];
	track_box_center.center_xy_wh[2] = rect_size[0];
	track_box_center.center_xy_wh[3] = rect_size[1];

	return track_box_center;
}
