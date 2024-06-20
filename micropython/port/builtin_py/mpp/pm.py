from mpp import pm_struct
from mpp import common_struct
from umpp import pm_api
from mpp.pm_struct import PM_GOVERNOR_MANUAL
from mpp.pm_struct import PM_GOVERNOR_PERFORMANCE
from mpp.pm_struct import PM_GOVERNOR_ENERGYSAVING
from mpp.pm_struct import PM_GOVERNOR_AUTO

class pm_domain:
    def __init__(self, domain):
        self.__domain = domain
        self.__profiles = []

    def get_freq(self):
        if len(self.__profiles) == 0:
            self.list_profiles()
        index = self.get_profile()
        return self.__profiles[index][0]

    def list_profiles(self):
        count = common_struct.k_u32_ptr()
        prifiles = pm_struct.k_pm_profile()
        ret = pm_api.kd_mpi_pm_get_profiles(self.__domain, count, prifiles)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_get_profiles ret: %d" % ret)
        buf = bytearray(pm_struct.k_pm_profile_size * count.value)
        prifiles = pm_struct.k_pm_profile(buf)
        ret = pm_api.kd_mpi_pm_get_profiles(self.__domain, count, prifiles)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_get_profiles ret: %d" % ret)
        self.__profiles.clear()
        for i in range(count.value):
            prifiles = pm_struct.k_pm_profile(buf, i)
            self.__profiles.append([prifiles.freq, prifiles.volt])
        return self.__profiles

    def get_governor(self):
        index = common_struct.k_s32_ptr()
        ret = pm_api.kd_mpi_pm_get_governor(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_get_governor ret: %d" % ret)
        return index.value

    def set_governor(self, index):
        ret = pm_api.kd_mpi_pm_set_governor(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_set_governor ret: %d" % ret)

    def get_profile(self):
        index = common_struct.k_s32_ptr()
        ret = pm_api.kd_mpi_pm_get_profile(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_get_profile ret: %d" % ret)
        return index.value

    def set_profile(self, index):
        ret = pm_api.kd_mpi_pm_set_profile(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_set_profile ret: %d" % ret)

    def set_profile_lock(self, index):
        ret = pm_api.kd_mpi_pm_set_profile_lock(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_set_profile_lock ret: %d" % ret)

    def set_profile_unlock(self, index):
        ret = pm_api.kd_mpi_pm_set_profile_unlock(self.__domain, index)
        if ret != 0:
            raise RuntimeError("call kd_mpi_pm_set_profile_unlock ret: %d" % ret)

cpu = pm_domain(pm_struct.PM_DOMAIN_CPU)
kpu = pm_domain(pm_struct.PM_DOMAIN_KPU)
