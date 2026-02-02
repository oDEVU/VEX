#include "HardwareInfo.hpp"

namespace vex {

std::string HardwareInfo::GPUName = "Unknown GPU";

std::string HardwareInfo::GetGPUName(){
    return GPUName;
}

void HardwareInfo::SetGPUName(const std::string& name){
    GPUName = name;
}

bool HardwareInfo::CheckHardwareSupport() {
    int info[4];
    CpuId(info, 1);

    bool osUsesXSAVE_XRSTORE = (info[2] & (1 << 27)) != 0;
    bool cpuHasAVX = (info[2] & (1 << 28)) != 0;

    if (!osUsesXSAVE_XRSTORE || !cpuHasAVX) return false;

    unsigned long long xcrFeatureMask = 0;
    #ifdef _MSC_VER
        xcrFeatureMask = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
    #else
        __asm__ ("xgetbv" : "=A" (xcrFeatureMask) : "c" (0));
    #endif

    if ((xcrFeatureMask & 0x6) != 0x6) return false;

    CpuId(info, 7);
    bool cpuHasAVX2 = (info[1] & (1 << 5)) != 0;

    return cpuHasAVX2;
}

bool HardwareInfo::HasAVX2() {
    static bool supported = CheckHardwareSupport();
    return supported;
}

} // namespace vex
