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

DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_reg)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_pm_get_reg, uint32_t)
DEF_INT_FUNC_INT_STRUCTPTR_STRUCTPTR(kd_mpi_pm_get_profiles, uint32_t, k_pm_profile)
DEF_INT_FUNC_INT(kd_mpi_pm_get_stat)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_governor)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_pm_get_governor, k_pm_governor)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_profile)
DEF_INT_FUNC_INT_STRUCTPTR(kd_mpi_pm_get_profile, int32_t)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_profile_lock)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_profile_unlock)
DEF_INT_FUNC_INT_INT_INT(kd_mpi_pm_set_thermal_protect)
DEF_INT_FUNC_INT_STRUCTPTR_STRUCTPTR(kd_mpi_pm_get_thermal_protect, int32_t, int32_t)
DEF_INT_FUNC_INT(kd_mpi_pm_set_thermal_shutdown)
DEF_INT_FUNC_STRUCTPTR(kd_mpi_pm_get_thermal_shutdown, int32_t)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_clock)
DEF_INT_FUNC_INT_INT(kd_mpi_pm_set_power)
