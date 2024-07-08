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
#include <vector>
#include <math.h>
#include <algorithm>
#include "aidemo_wrap.h"

using std::vector;
const float PI = 3.1415926;

void eye_gaze_softmax(vector<float>& input,vector<float>& output)
{
    std::vector<float>::iterator p_input_max = std::max_element(input.begin(), input.end());
    float input_max = *p_input_max;
    float input_total = 0;
    
    for(auto x:input)
	{
		input_total+=exp( x- input_max);
	}

    output.resize(input.size());
	for(int i=0;i<input.size();++i)
	{
		output[i] = exp(input[i] - input_max)/input_total;
	}
}

void eye_gaze_post_process(float** p_outputs_,float* pitch,float* yaw)
{
	for(int out_index = 0;out_index < 2; ++out_index)
	{
		vector<float> pred(p_outputs_[out_index],p_outputs_[out_index] + 90);
		vector<float> softmax_pred;
		eye_gaze_softmax(pred,softmax_pred);
		float pred_sum = 0;
		for(int i = 0;i<softmax_pred.size();++i)
		{
			pred_sum += softmax_pred[i] * i;
		}
		pred_sum = pred_sum * 4 - 180;
		pred_sum = pred_sum * PI / 180.0;
		if(out_index == 0)
			*pitch = pred_sum;
		else
			*yaw = pred_sum;
	}
}





