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

DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_ai_set_pub_attr, k_aio_dev_attr)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_ai_get_pub_attr, k_aio_dev_attr)
DEF_INT_FUNC_INT(kd_mpi_ai_enable)
DEF_INT_FUNC_INT(kd_mpi_ai_disable)
DEF_INT_FUNC_INT_INT(kd_mpi_ai_enable_chn)
DEF_INT_FUNC_INT_INT(kd_mpi_ai_disable_chn)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_set_chn_param, k_ai_chn_param)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_get_chn_param, k_ai_chn_param)
DEF_INT_FUNC_INT_INT(kd_mpi_ai_get_fd)
DEF_INT_FUNC_INT_INT_STRUCTPTR_INT(kd_mpi_ai_get_frame, k_audio_frame)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_release_frame, k_audio_frame)
DEF_INT_FUNC_INT_INT_INT(kd_mpi_ai_set_vqe_attr)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_get_vqe_attr, k_bool)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_set_pitch_shift_attr, k_ai_chn_pitch_shift_param)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_ai_get_pitch_shift_attr, k_ai_chn_pitch_shift_param)
