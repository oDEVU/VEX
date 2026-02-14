#include "components/HardwareInfo.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <algorithm>

#ifdef _WIN32
    #include <io.h>
    #define WRITE_FUNC _write
#else
    #include <unistd.h>
    #define WRITE_FUNC write
#endif

#define V_MAJOR(v) ((v) >> 22)
#define V_MINOR(v) (((v) >> 12) & 0x3FF)
#define V_PATCH(v) ((v) & 0xFFF)

namespace vex {

std::string HardwareInfo::CPUName = "";
std::string HardwareInfo::SystemMemory = "";
std::string HardwareInfo::GPUName = "Unknown GPU";
std::string HardwareInfo::DriverVersion = "Unknown";
std::string HardwareInfo::VulkanDeviceVersion = "0.0.0";
std::string HardwareInfo::VulkanRequestedVersion = "0.0.0";
VulkanFeatures HardwareInfo::GPUFeatures = {};

void HardwareInfo::PrintCrashDump(int fd) {
    if (fd < 0) return;

    auto safe_write = [fd](const char* label, const std::string& val) {
        if (!label) return;
        WRITE_FUNC(fd, label, strlen(label));
        if (!val.empty()) {
            WRITE_FUNC(fd, val.c_str(), val.length());
        } else {
            WRITE_FUNC(fd, "Unknown", 7);
        }
        WRITE_FUNC(fd, "\n", 1);
    };

    auto safe_write_bool = [fd](const char* label, bool val) {
        WRITE_FUNC(fd, label, strlen(label));
        if (val) WRITE_FUNC(fd, "YES\n", 4);
        else WRITE_FUNC(fd, "NO\n", 3);
    };

    const char* header = "\n=== SYSTEM INFO ===\n";
    WRITE_FUNC(fd, header, strlen(header));

    if(CPUName.empty()) DetectCPUName();
    if(SystemMemory.empty()) DetectSystemMemory();

    safe_write("CPU:                 ", CPUName);
    safe_write("RAM:                 ", SystemMemory);
    safe_write_bool("AVX2:                ", HasAVX2());

    safe_write("GPU:                 ", GPUName);
    safe_write("Driver:              ", DriverVersion);
    safe_write("Vulkan Device:       ", VulkanDeviceVersion);
    safe_write("Vulkan Requested:    ", VulkanRequestedVersion);

    safe_write_bool("Feat: MultiDraw:     ", GPUFeatures.multiDraw);
    safe_write_bool("Feat: Indirect:      ", GPUFeatures.indirectDraw);
    safe_write_bool("Feat: Bindless:      ", GPUFeatures.bindlessTextures);
    safe_write_bool("Feat: Shader Params: ", GPUFeatures.shaderDrawParameters);
}

static std::string DecodeDriverVersion(uint32_t vendorID, uint32_t v) {
    if (vendorID == 0x10DE) {
        int major = (v >> 22) & 0x3FF;
        int minor = (v >> 14) & 0xFF;
        int sub   = (v >> 6)  & 0xFF;
        int patch = v & 0x3F;
        std::stringstream ss;
        ss << major << "." << minor << "." << sub << "." << patch;
        return ss.str();
    }

#if defined(_WIN32)
    if (vendorID == 0x8086) {
        int major = (v >> 14);
        int minor = v & 0x3FFF;
        std::stringstream ss;
        ss << major << "." << minor;
        return ss.str();
    }
#endif

    std::stringstream ss;
    ss << V_MAJOR(v) << "." << V_MINOR(v) << "." << V_PATCH(v);
    return ss.str();
}

void HardwareInfo::SetGPUInfo(const std::string& name, uint32_t vendorID, uint32_t driverVersion) {
    GPUName = name;
    DriverVersion = DecodeDriverVersion(vendorID, driverVersion);
}
std::string HardwareInfo::GetGPUName() { return GPUName; }
std::string HardwareInfo::GetDriverVersion() { return DriverVersion; }

void HardwareInfo::SetVulkanVersions(const std::string& deviceVer, const std::string& requestedVer) {
    VulkanDeviceVersion = deviceVer;
    VulkanRequestedVersion = requestedVer;
}
std::string HardwareInfo::GetVulkanDeviceVersion() { return VulkanDeviceVersion; }
std::string HardwareInfo::GetVulkanRequestedVersion() { return VulkanRequestedVersion; }

void HardwareInfo::SetVulkanFeatures(const VulkanFeatures& features) {
    GPUFeatures = features;
}
VulkanFeatures HardwareInfo::GetVulkanFeatures() { return GPUFeatures; }

std::string HardwareInfo::GetCPUName() {
    if (CPUName.empty()) DetectCPUName();
    return CPUName;
}

std::string HardwareInfo::GetSystemMemory() {
    if (SystemMemory.empty()) DetectSystemMemory();
    return SystemMemory;
}

void HardwareInfo::DetectCPUName() {
    int info[4] = { -1 };
    char brand[0x40];
    memset(brand, 0, sizeof(brand));

    CpuId(info, 0x80000000);
    unsigned int nExIds = info[0];

    if (nExIds >= 0x80000004) {
        std::vector<int> cpuIds;
        for (int i = 0x80000002; i <= 0x80000004; ++i) {
            CpuId(info, i);
            cpuIds.push_back(info[0]);
            cpuIds.push_back(info[1]);
            cpuIds.push_back(info[2]);
            cpuIds.push_back(info[3]);
        }
        memcpy(brand, cpuIds.data(), sizeof(brand));
        CPUName = std::string(brand);
    } else {
        CPUName = "Generic x86 CPU";
    }

    CPUName.erase(CPUName.find_last_not_of(' ') + 1);
    CPUName.erase(0, CPUName.find_first_not_of(' '));
}

void HardwareInfo::DetectSystemMemory() {
    uint64_t totalRam = 0;
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    totalRam = status.ullTotalPhys;
#else
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    totalRam = pages * page_size;
#endif

    double gb = static_cast<double>(totalRam) / (1024.0 * 1024.0 * 1024.0);
    std::stringstream ss;
    if (gb >= 1.0) {
        ss << std::fixed << std::setprecision(1) << gb << " GB";
    } else {
        ss << (totalRam / (1024 * 1024)) << " MB";
    }
    SystemMemory = ss.str();
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
