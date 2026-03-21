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
    /// @brief Holds the status of various Vulkan features.
    struct VulkanFeatures {
        bool multiDraw = false;            ///< @brief Indicates if VK_EXT_multi_draw is supported.
        bool indirectDraw = false;         ///< @brief Indicates if indirect draw calls are supported.
        bool bindlessTextures = false;     ///< @brief Indicates if bindless textures (descriptor indexing) are supported.
        bool shaderDrawParameters = false; ///< @brief Indicates if VK_KHR_shader_draw_parameters is supported.
    };

    /// @brief Utility class for retrieving and storing hardware and driver information.
    class HardwareInfo {
    public:
        /// @brief Retrieves the name of the CPU.
        /// @return std::string - The CPU model name.
        static std::string GetCPUName();

        /// @brief Retrieves the total amount of system memory.
        /// @return std::string - The total system memory formatted as a string.
        static std::string GetSystemMemory();

        /// @brief Checks if the CPU supports the AVX2 instruction set.
        /// @return bool - True if AVX2 is supported, false otherwise.
        static bool HasAVX2();

        /// @brief Sets the stored GPU information.
        /// @param name The name of the GPU device.
        /// @param vendorID The vendor ID of the GPU.
        /// @param driverVersion The driver version of the GPU.
        static void SetGPUInfo(const std::string& name, uint32_t vendorID, uint32_t driverVersion);

        /// @brief Retrieves the name of the GPU.
        /// @return std::string - The GPU name.
        static std::string GetGPUName();

        /// @brief Retrieves the GPU driver version.
        /// @return std::string - The driver version string.
        static std::string GetDriverVersion();

        /// @brief Sets the Vulkan API versions.
        /// @param deviceVer The version supported by the device.
        /// @param requestedVer The version requested by the engine.
        static void SetVulkanVersions(const std::string& deviceVer, const std::string& requestedVer);

        /// @brief Retrieves the Vulkan version supported by the device.
        /// @return std::string - The device Vulkan version.
        static std::string GetVulkanDeviceVersion();

        /// @brief Retrieves the Vulkan version requested by the engine.
        /// @return std::string - The requested Vulkan version.
        static std::string GetVulkanRequestedVersion();

        /// @brief Sets the active Vulkan features.
        /// @param features The Vulkan features structure.
        static void SetVulkanFeatures(const VulkanFeatures& features);

        /// @brief Retrieves the active Vulkan features.
        /// @return VulkanFeatures - The currently stored Vulkan features.
        static VulkanFeatures GetVulkanFeatures();

        /// @brief Prints hardware information to a crash dump file descriptor.
        /// @param fd The file descriptor to write to.
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
