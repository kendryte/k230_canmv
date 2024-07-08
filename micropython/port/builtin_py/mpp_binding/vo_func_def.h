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

DEF_INT_FUNC_VOID(kd_display_set_backlight)
DEF_INT_FUNC_VOID(kd_display_reset)
DEF_INT_FUNC_VOID(kd_mpi_dsi_set_test_pattern)
DEF_INT_FUNC_VOID(kd_mpi_vo_enable)
DEF_INT_FUNC_VOID(kd_mpi_vo_disable)
DEF_INT_FUNC_VOID(kd_mpi_vo_init)
DEF_INT_FUNC_VOID(kd_mpi_vo_enable_wbc)
DEF_INT_FUNC_VOID(kd_mpi_vo_disable_wbc)
DEF_INT_FUNC_INT(kd_mpi_dsi_enable)
DEF_INT_FUNC_INT(kd_mpi_vo_osd_enable)
DEF_INT_FUNC_INT(kd_mpi_vo_osd_disable)
DEF_INT_FUNC_INT(kd_mpi_vo_enable_video_layer)
DEF_INT_FUNC_INT(kd_mpi_vo_disable_video_layer)
DEF_INT_FUNC_INT_INT(kd_mpi_vo_set_user_sync_info)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_dsi_set_attr, k_vo_dsi_attr)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_set_mipi_phy_attr, k_vo_mipi_phy_attr)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vo_set_dev_param, k_vo_pub_attr)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vo_draw_frame, k_vo_draw_frame)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vo_set_wbc_attr, k_vo_wbc_attr)
DEF_INT_FUNC_ARRAY_INT(kd_mpi_dsi_send_cmd, k_u8)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vo_set_video_layer_attr, k_vo_video_layer_attr)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vo_set_video_osd_attr, k_vo_video_osd_attr)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vo_chn_insert_frame, k_video_frame_info)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vo_chn_dump_release, k_video_frame_info)
DEF_INT_FUNC_INT_INT_STRUCTPTR(kd_mpi_dsi_read_pkg, k_u32)
DEF_INT_FUNC_INT_STRUCTPTR_INT(kd_mpi_vo_chn_dump_frame, k_video_frame_info)
