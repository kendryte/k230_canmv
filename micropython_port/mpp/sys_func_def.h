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

DEF_INT_FUNC_INT_INT(kd_mpi_sys_mmap)
DEF_INT_FUNC_INT_INT(kd_mpi_sys_mmap_cached)
DEF_INT_FUNC_INT_INT(kd_mpi_sys_munmap)
DEF_INT_FUNC_INT_INT_INT(kd_mpi_sys_mmz_flush_cache)
DEF_INT_FUNC_INT_INT(kd_mpi_sys_mmz_free)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_sys_get_virmem_info, k_sys_virmem_info)
DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(kd_mpi_sys_bind, k_mpp_chn, k_mpp_chn)
DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(kd_mpi_sys_unbind, k_mpp_chn, k_mpp_chn)
DEF_INT_FUNC_STRUCTPTR_STRUCTPTR(kd_mpi_sys_get_bind_by_dest, k_mpp_chn, k_mpp_chn)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_log_set_level_conf, k_log_level_conf)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_log_get_level_conf, k_log_level_conf)
DEF_INT_FUNC_INT(kd_mpi_log_set_wait_flag)
DEF_INT_FUNC_ARRAY_INT(kd_mpi_log_read, k_char)
DEF_VOID_FUNC_VOID(kd_mpi_log_close)
DEF_INT_FUNC_INT(kd_mpi_log_set_console)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_log_get_console, k_bool)
