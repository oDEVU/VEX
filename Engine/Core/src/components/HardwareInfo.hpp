#pragma once
#include <array>
#include <cstdint>

#ifdef _MSC_VER
    #include <intrin.h>
#else
    #include <cpuid.h>
#endif

namespace vex {
    class HardwareInfo {
    public:
        static bool HasAVX2() {
            static bool supported = CheckHardwareSupport();
            return supported;
        }

    private:
        static void CpuId(int info[4], int function_id) {
            #ifdef _MSC_VER
                __cpuidex(info, function_id, 0);
            #else
                unsigned int eax, ebx, ecx, edx;
                __cpuid_count(function_id, 0, eax, ebx, ecx, edx);
                info[0] = static_cast<int>(eax);
                info[1] = static_cast<int>(ebx);
                info[2] = static_cast<int>(ecx);
                info[3] = static_cast<int>(edx);
            #endif
        }

        static bool CheckHardwareSupport() {
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
    };
}
