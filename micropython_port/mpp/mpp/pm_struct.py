import uctypes
from mpp import pm_def

PM_DOMAIN_CPU = const(0)
PM_DOMAIN_KPU = const(1)
PM_DOMAIN_DPU = const(2)
PM_DOMAIN_VPU = const(3)
PM_DOMAIN_DISPLAY = const(4)
PM_DOMAIN_MEDIA = const(5)

PM_GOVERNOR_MANUAL = const(0)
PM_GOVERNOR_PERFORMANCE = const(1)
PM_GOVERNOR_ENERGYSAVING = const(2)
PM_GOVERNOR_AUTO = const(3)

k_pm_profile_size = uctypes.sizeof(pm_def.k_pm_profile_desc)

def k_pm_profile(buf=None, idx=0):
    layout= uctypes.NATIVE
    addr = 0 if buf == None else uctypes.addressof(buf) + idx * k_pm_profile_size
    s = uctypes.struct(addr, pm_def.k_pm_profile_desc, layout)
    return s
