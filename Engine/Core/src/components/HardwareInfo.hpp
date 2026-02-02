#pragma once
#include <array>
#include <cstdint>
#include <string>

#ifdef _MSC_VER
    #include <intrin.h>
#else
    #include <cpuid.h>
#endif

namespace vex {
    class HardwareInfo {
    public:
        static bool HasAVX2();
        static std::string GetGPUName();
        static void SetGPUName(const std::string& name);

    private:
        static std::string GPUName;

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

        static bool CheckHardwareSupport();
    };
}
