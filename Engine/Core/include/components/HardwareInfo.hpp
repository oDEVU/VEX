#pragma once
#include <array>
#include <cstdint>
#include <string>

#ifdef _MSC_VER
    #include <windows.h>
    #include <intrin.h>
#else
    #include <unistd.h>
    #include <cpuid.h>
#endif

namespace vex {
    struct VulkanFeatures {
        bool multiDraw = false;
        bool indirectDraw = false;
        bool bindlessTextures = false;
        bool shaderDrawParameters = false;
    };

    class HardwareInfo {
    public:
        static std::string GetCPUName();
        static std::string GetSystemMemory();
        static bool HasAVX2();

        static void SetGPUInfo(const std::string& name, uint32_t vendorID, uint32_t driverVersion);
        static std::string GetGPUName();
        static std::string GetDriverVersion();

        static void SetVulkanVersions(const std::string& deviceVer, const std::string& requestedVer);
        static std::string GetVulkanDeviceVersion();
        static std::string GetVulkanRequestedVersion();

        static void SetVulkanFeatures(const VulkanFeatures& features);
        static VulkanFeatures GetVulkanFeatures();

        static void PrintCrashDump(int fd);
    private:
        static std::string CPUName;
        static std::string SystemMemory;

        static std::string GPUName;
        static std::string DriverVersion;
        static std::string VulkanDeviceVersion;
        static std::string VulkanRequestedVersion;
        static VulkanFeatures GPUFeatures;

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

        static void DetectCPUName();
        static void DetectSystemMemory();

        static bool CheckHardwareSupport();
    };
}
