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
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include "aidemo_wrap.h"

using std::vector;

int input_shapes_[] = {1,3,120,120};
int post_ver_dim_ = 38365;

void from_numpy(cv_and_ndarray_convert_info *info,cv::Mat& mat_data);
void similar_transform(Bbox& roi,float* vertices)
{
    double scale_x = (roi.w) / input_shapes_[3];
    double scale_y = (roi.h) / input_shapes_[3];
    double s = (scale_x + scale_y) / 2;

    for (int row_index = 0; row_index < 3; ++row_index)
    {
        if (row_index == 0)
        {
            int index = 0;
            for (int col_index = 0; col_index < post_ver_dim_; ++col_index)
            {
                index = row_index * post_ver_dim_ + col_index;
                vertices[index] -= 1;
                vertices[index] = vertices[index] * scale_x + roi.x;
            }
        }
        else if (row_index == 2)
        {
            int index = 0;
            float min_dim2 = 0;
            for (int col_index = 0; col_index < post_ver_dim_; ++col_index)
            {
                index = row_index * post_ver_dim_ + col_index;
                vertices[index] -= 1;
                vertices[index] *= s;
                if (col_index == 0)
                    min_dim2 = vertices[index];
                else
                {
                    if (vertices[index] < min_dim2)
                        min_dim2 = vertices[index];
                }
            }
            for (int col_index = 0; col_index < post_ver_dim_; ++col_index)
            {
                vertices[row_index * post_ver_dim_ + col_index] -= min_dim2;
            }
        }
        else
        {
            int index = 0;
            for (int col_index = 0; col_index < post_ver_dim_; ++col_index)
            {
                index = row_index * post_ver_dim_ + col_index;
                vertices[index] = input_shapes_[3] - vertices[index];
                vertices[index] = vertices[index] * scale_y + roi.y;
            }
        }
    }
}

void recon_vers(Bbox& roi_box_lst,float* vertices)
{
    similar_transform(roi_box_lst,vertices);
}

void face_mesh_post_process(Bbox roi,generic_array* p_vertices)
{ 
    float *p_data = (float*)p_vertices->data_;
    recon_vers(roi, p_data);
}

void draw_mesh(cv_and_ndarray_convert_info *in_info, generic_array* p_vertices)
{
    cv::Mat src_img;
    from_numpy(in_info,src_img);

    float *p_data = (float*)p_vertices->data_;

    int x,y;
    for(int ver_index =0;ver_index < post_ver_dim_;ver_index = ver_index + 6*2)
    {
        x = p_data[ver_index];
        y = p_data[ver_index + post_ver_dim_];
        cv::circle(src_img, cv::Point(x, y), 2, cv::Scalar(200, 0, 0, 255), 4); 
    }
}



