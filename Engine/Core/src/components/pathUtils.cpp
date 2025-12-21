#include "components/pathUtils.hpp"
#include "components/errorUtils.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/param.h>
#else
#include <unistd.h>
#endif

namespace vex {

static std::string g_AssetRootOverride = "";

void SetAssetRoot(const std::string& projectPath) {
    g_AssetRootOverride = projectPath;
}

std::string GetAssetDir() {
    return g_AssetRootOverride;
}

std::filesystem::path GetExecutableDir() {
    std::filesystem::path exePath;

#ifdef _WIN32
    wchar_t wPath[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, wPath, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        vex::throw_error("Failed to get executable path on Windows");
    }
    int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wPath, -1, nullptr, 0, nullptr, nullptr);
    if (utf8Size == 0) {
        vex::throw_error("Failed to convert executable path to UTF-8");
    }
    std::string utf8Path(utf8Size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wPath, -1, utf8Path.data(), utf8Size, nullptr, nullptr);
    exePath = std::filesystem::path(utf8Path);
#elif defined(__APPLE__)
    char pathBuf[MAXPATHLEN];
    uint32_t size = sizeof(pathBuf);
    if (_NSGetExecutablePath(pathBuf, &size) != 0) {
        vex::throw_error("Failed to get executable path on macOS");
    }
    exePath = std::filesystem::canonical(pathBuf);
#else
    char pathBuf[1024];
    ssize_t len = ::readlink("/proc/self/exe", pathBuf, sizeof(pathBuf) - 1);
    if (len == -1) {
        vex::throw_error("Failed to read /proc/self/exe on Linux");
    }
    pathBuf[len] = '\0';
    exePath = std::filesystem::path(pathBuf);
#endif

    return exePath.parent_path();
}

std::string GetAssetPath(const std::string& relativePath) {

    const std::string& overridePath = g_AssetRootOverride;
    if (!overridePath.empty()) {
        if(relativePath.contains(overridePath)){
            return relativePath;
        }
        return (std::filesystem::path(overridePath) / relativePath).string();
    }

    #if DEBUG
        static std::filesystem::path exeDir = GetExecutableDir();
        return (exeDir / relativePath).string();
    #else
        return relativePath;
    #endif
}

} // namespace vex
