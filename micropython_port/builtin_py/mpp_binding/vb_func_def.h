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

DEF_INT_FUNC_STRUCTPTR(kd_mpi_vb_create_pool, k_vb_pool_config)
DEF_INT_FUNC_INT(kd_mpi_vb_destory_pool)
DEF_INT_FUNC_INT_INT_STR(kd_mpi_vb_get_block)
DEF_INT_FUNC_INT(kd_mpi_vb_release_block)
DEF_INT_FUNC_INT(kd_mpi_vb_phyaddr_to_handle)
DEF_INT_FUNC_INT(kd_mpi_vb_handle_to_phyaddr)
DEF_INT_FUNC_INT(kd_mpi_vb_handle_to_pool_id)
DEF_INT_FUNC_INT(kd_mpi_vb_inquire_user_cnt)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vb_get_supplement_attr, k_video_supplement)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vb_set_supplement_config, k_vb_supplement_config)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vb_get_supplement_config, k_vb_supplement_config)
DEF_INT_FUNC_VOID(kd_mpi_vb_init)
DEF_INT_FUNC_VOID(kd_mpi_vb_exit)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vb_set_config, k_vb_config)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_vb_get_config, k_vb_config)
DEF_INT_FUNC_INT(kd_mpi_vb_init_mod_common_pool)
DEF_INT_FUNC_INT(kd_mpi_vb_exit_mod_common_pool)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vb_set_mod_pool_config, k_vb_config)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_vb_get_mod_pool_config, k_vb_config)

DEF_INT_FUNC_STRUCTPTR(vb_mgmt_get_block, vb_block_info)
DEF_INT_FUNC_STRUCTPTR(vb_mgmt_put_block, vb_block_info)

DEF_INT_FUNC_INT(vb_mgmt_vicap_dev_inited)
DEF_INT_FUNC_INT(vb_mgmt_vicap_dev_deinited)

DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(vb_mgmt_push_link_info, k_mpp_chn, k_mpp_chn)
DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(vb_mgmt_pop_link_info, k_mpp_chn, k_mpp_chn)
