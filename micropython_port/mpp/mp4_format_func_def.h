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

DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(kd_mp4_create, void*, k_mp4_config_s)
DEF_INT_FUNC_INT(kd_mp4_destroy)
DEF_INT_FUNC_INT_STRUCTPTR_STRUCTPTR(kd_mp4_create_track, void*, k_mp4_track_info_s)
DEF_INT_FUNC_INT(kd_mp4_destroy_tracks)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mp4_write_frame, k_mp4_frame_data_s)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mp4_get_file_info, k_mp4_file_info_s)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mp4_get_track_by_index, k_mp4_track_info_s)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mp4_get_frame, k_mp4_frame_data_s)