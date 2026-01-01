#include "CrashDecoder.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace fs = std::filesystem;

inline bool starts_with(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

bool CrashDecoder::loadLog(const std::string& logPath) {
    std::ifstream file(logPath);
    if (!file.is_open()) return false;

    std::string line;
    bool inCrashSection = false;
    bool inMemoryMap = false;
    std::string pendingBase;

    while (std::getline(file, line)) {
        if (line.find("========== CRASH OCCURRED ==========") != std::string::npos) {
            inCrashSection = true;
            crash_ = CrashInfo{};
            continue;
        }

        if (!inCrashSection) continue;

        if (starts_with(line, "Reason:")) {
            size_t colon = line.find(':');
            if (colon != std::string::npos) {
                crash_.reason = line.substr(colon + 1);
                crash_.reason.erase(0, crash_.reason.find_first_not_of(" \t"));
            }
        }

        if (starts_with(line, "Absolute Crash Address:") || line.find("RIP:") != std::string::npos) {
            size_t pos = line.find("0x");
            if (pos != std::string::npos) {
                const char* hexStart = line.c_str() + pos + 2;
                crash_.crash_address = std::strtoull(hexStart, nullptr, 16);
            }
        }

        if (line.find("--- MEMORY MAP (Base Addresses) ---") != std::string::npos ||
            line.find("--- MEMORY MAP ---") != std::string::npos) {
            inMemoryMap = true;
            pendingBase.clear();
            continue;
        }

        if (inMemoryMap && line.find("-----------------------------------") != std::string::npos) {
            inMemoryMap = false;
            pendingBase.clear();
            continue;
        }

        if (!inMemoryMap) continue;

        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        std::string trimmed = line.substr(start, end - start + 1);

#ifdef _WIN32
        if (starts_with(trimmed, "Base:")) {
            size_t pipePos = trimmed.find('|');
            if (pipePos != std::string::npos) {
                std::string addrPart = trimmed.substr(0, pipePos);
                std::string pathPart = trimmed.substr(pipePos + 1);
                pathPart.erase(0, pathPart.find_first_not_of(" \t"));

                size_t hexPos = addrPart.find("0x");
                if (hexPos != std::string::npos) {
                    uintptr_t base = std::strtoull(addrPart.c_str() + hexPos + 2, nullptr, 16);
                    ModuleInfo mod;
                    mod.base = base;
                    mod.end = base + 0xFFFFFFFFFFFFFFFFULL;
                    mod.path = pathPart;
                    size_t sep = pathPart.find_last_of("\\/");
                    mod.name = (sep == std::string::npos) ? pathPart : pathPart.substr(sep + 1);
                    crash_.modules.push_back(mod);
                }
            } else {
                size_t hexPos = trimmed.find("0x");
                if (hexPos != std::string::npos) {
                    pendingBase = trimmed.substr(hexPos);
                }
            }
        } else if (starts_with(trimmed, "|") && !pendingBase.empty()) {
            std::string pathPart = trimmed.substr(1);
            pathPart.erase(0, pathPart.find_first_not_of(" \t"));

            uintptr_t base = std::strtoull(pendingBase.c_str(), nullptr, 16);
            if (base != 0) {
                ModuleInfo mod;
                mod.base = base;
                mod.end = base + 0xFFFFFFFFFFFFFFFFULL;
                mod.path = pathPart;
                size_t sep = pathPart.find_last_of("\\/");
                mod.name = (sep == std::string::npos) ? pathPart : pathPart.substr(sep + 1);
                crash_.modules.push_back(mod);
            }
            pendingBase.clear();
        }
#else
        std::istringstream iss(trimmed);
        std::string addrRange;
        if (std::getline(iss, addrRange, ' ')) {
            size_t dash = addrRange.find('-');
            if (dash != std::string::npos) {
                std::string baseStr = addrRange.substr(0, dash);
                std::string endStr = addrRange.substr(dash + 1);

                uintptr_t base = std::strtoull(baseStr.c_str(), nullptr, 16);
                uintptr_t end = std::strtoull(endStr.c_str(), nullptr, 16);

                std::string perm, offset, dev, inode;
                iss >> perm >> offset >> dev >> inode;

                std::string path;
                std::getline(iss, path);
                if (!path.empty() && path[0] == ' ') path.erase(0, 1);
                if (path.empty()) path = "(anonymous)";

                if (base != 0 && end != 0) {
                    ModuleInfo mod;
                    mod.base = base;
                    mod.end = end;
                    mod.path = path;
                    size_t sep = path.find_last_of('/');
                    mod.name = (sep == std::string::npos) ? path : path.substr(sep + 1);
                    crash_.modules.push_back(mod);
                }
            }
        }
#endif
    }

    return crash_.crash_address != 0;
}

bool CrashDecoder::loadSymbolsFromFolder(const std::string& folder) {
    if (!fs::exists(folder)) return false;

    for (const auto& entry : fs::recursive_directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
#ifdef _WIN32
        if (_stricmp(ext.c_str(), ".pdb") == 0)
#else
        if (ext == ".debug")
#endif
        {
            std::string filename = entry.path().filename().string();
            size_t dot = filename.find_last_of('.');
            std::string moduleName = (dot != std::string::npos) ? filename.substr(0, dot) : filename;

#ifdef __linux__
            if (starts_with(moduleName, "lib")) {
                moduleName = moduleName.substr(3);
            }
#endif
            moduleToSymbolFile_[moduleName] = entry.path().string();
        }
    }
    return !moduleToSymbolFile_.empty();
}

std::string CrashDecoder::findContainingModule(uintptr_t addr) const {
#ifdef _WIN32
    // On Windows: no end address, so find the module with the highest base <= addr
    uintptr_t bestBase = 0;
    std::string bestName;
    for (const auto& mod : crash_.modules) {
        if (mod.base <= addr && mod.base > bestBase) {
            bestBase = mod.base;
            bestName = mod.name;
        }
    }
    return bestName;
#else
    // Linux: proper range check
    for (const auto& mod : crash_.modules) {
        if (addr >= mod.base && addr < mod.end) {
            return mod.name;
        }
    }
    return "";
#endif
}

std::string CrashDecoder::resolveLinux(const std::string& symbolFile, uintptr_t offset) const {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "addr2line -e \"%s\" -f -C 0x%llx", symbolFile.c_str(), (unsigned long long)offset);

    FILE* pipe = POPEN(cmd, "r");
    if (!pipe) return "?? (addr2line failed)";

    char func[512] = {};
    char file[512] = {};
    std::string result = "??";
    if (fgets(func, sizeof(func), pipe)) {
        func[strcspn(func, "\r\n")] = 0;
        if (fgets(file, sizeof(file), pipe)) {
            file[strcspn(file, "\r\n")] = 0;
            if (func[0] != '?' || func[1] != '?') {
                result = std::string(func) + " @ " + file;
            }
        }
    }
    PCLOSE(pipe);
    return result;
}

std::string CrashDecoder::resolveWindows(const std::string& pdbFile, uintptr_t offset) const {
#ifdef _WIN32
    HANDLE hProcess = GetCurrentProcess();

    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEBUG);

    std::string searchPath;
    size_t lastSlash = pdbFile.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        searchPath = pdbFile.substr(0, lastSlash);
    }

    if (!SymInitialize(hProcess, searchPath.c_str(), FALSE)) {
        return "?? (SymInitialize failed)";
    }

    std::string binaryPath;
    if (!binariesFolder_.empty()) {
        std::string moduleName = fs::path(pdbFile).stem().string();
        binaryPath = binariesFolder_ + "\\" + moduleName + ".dll";
        if (!fs::exists(binaryPath)) {
            binaryPath = binariesFolder_ + "\\" + moduleName + ".exe";
        }
        if (!fs::exists(binaryPath)) {
            binaryPath = "";
        }
    }

    DWORD64 moduleBase = 0;

    if (!binaryPath.empty()) {
        moduleBase = SymLoadModuleEx(hProcess, nullptr, binaryPath.c_str(), nullptr, 0, 0, nullptr, 0);

        if (moduleBase == 0) {
            SymCleanup(hProcess);
            return "?? (Failed to load binary)";
        }
    } else {
        moduleBase = SymLoadModuleEx(hProcess, nullptr, pdbFile.c_str(), nullptr, 0, 0, nullptr, 0);

        if (moduleBase == 0) {
            SymCleanup(hProcess);
            return "?? (No binary - failed to load PDB)";
        }
    }

    DWORD64 addrToLookup = moduleBase + offset;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    memset(buffer, 0, sizeof(buffer));
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    DWORD64 displacement = 0;
    std::string result = "??";

    if (SymFromAddr(hProcess, addrToLookup, &displacement, symbol)) {
        result = symbol->Name;

        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisp = 0;
        if (SymGetLineFromAddr64(hProcess, addrToLookup, &lineDisp, &line)) {
            result += " @ " + std::string(line.FileName) + ":" + std::to_string(line.LineNumber);
        }
    } else {
        DWORD error = GetLastError();
        result = "?? (SymFromAddr failed: " + std::to_string(error) + ")";
    }

    SymUnloadModule64(hProcess, moduleBase);
    SymCleanup(hProcess);
    return result;
#else
    return "??";
#endif
}

std::string CrashDecoder::decode() const {
    if (crash_.crash_address == 0) {
        return "No valid crash address found in log.";
    }

    std::ostringstream out;
    out << "========== DECODED CRASH REPORT ==========\n";
    out << "Reason: " << (crash_.reason.empty() ? "Unknown" : crash_.reason) << "\n";
    out << std::hex << std::showbase;
    out << "Crash Address: " << crash_.crash_address << "\n\n";

    out << "Detected " << crash_.modules.size() << " loaded modules:\n";
    for (const auto& m : crash_.modules) {
        out << "  [" << m.base << "] " << m.name << "\n";
    }
    out << "\n";

    std::string moduleName = findContainingModule(crash_.crash_address);
    if (moduleName.empty()) {
        out << "ERROR: Crash address not inside any known module.\n";
        return out.str();
    }

    auto it = moduleToSymbolFile_.find(moduleName);
    if (it == moduleToSymbolFile_.end()) {
        for (const auto& pair : moduleToSymbolFile_) {
            if (pair.first.find(moduleName) != std::string::npos ||
                moduleName.find(pair.first) != std::string::npos) {
                it = moduleToSymbolFile_.find(pair.first);
                break;
            }
        }
    }

    if (it == moduleToSymbolFile_.end()) {
        out << "No debug symbols found for module: " << moduleName << "\n";
        return out.str();
    }

    uintptr_t base = 0;
    for (const auto& m : crash_.modules) {
        if (m.name == moduleName) {
            base = m.base;
            break;
        }
    }

    uintptr_t offset = crash_.crash_address - base;

    out << "Crash occurred in module: " << moduleName << "\n";
    out << "Base Address: " << base << "\n";
    out << "Relative Offset: " << offset << "\n";
    out << "Symbol File: " << it->second << "\n\n";

#ifdef __linux__
    std::string location = resolveLinux(it->second, offset);
#else
    std::string location = resolveWindows(it->second, offset);
#endif

    out << "Source Location:\n>>> " << location << "\n";
    out << "==========================================\n";

    return out.str();
}
