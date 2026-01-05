#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

struct ModuleInfo {
    uintptr_t base;
    uintptr_t end;
    std::string path;
    std::string name;
};

struct CrashInfo {
    std::string reason;
    uintptr_t crash_address = 0;
    std::vector<ModuleInfo> modules;
};

class CrashDecoder {
public:
    bool loadLog(const std::string& logPath);
    bool loadSymbolsFromFolder(const std::string& symbolsFolder);
    std::string decode() const;

    const CrashInfo& getCrashInfo() const { return crash_; }

    void setBinariesFolder(const std::string& folder) { binariesFolder_ = folder; }

private:
    std::string binariesFolder_;
    CrashInfo crash_;
    std::unordered_map<std::string, std::string> moduleToSymbolFile_;

    std::string findContainingModule(uintptr_t addr) const;
    std::string resolveLinux(const std::string& symbolFile, uintptr_t offset) const;
    std::string resolveWindows(const std::string& pdbFile, uintptr_t offset) const;
};
