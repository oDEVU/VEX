#include "components/errorUtils.hpp"
#include "components/pathUtils.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <SDL3/SDL_log.h>
#include <vector>
#include <array>
#include <mutex>
#include <cstring>
#include <atomic>
#include <algorithm>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
    #include <io.h>
    #define WRITE_FUNC _write
    #define OPEN_FUNC _open
    #define CLOSE_FUNC _close
    #define READ_FUNC _read
    #define O_FLAGS _O_CREAT | _O_TRUNC | _O_WRONLY
    #define O_RDONLY _O_RDONLY
    #define S_FLAGS _S_IWRITE
#else
    #include <signal.h>
    #include <ucontext.h>
    #include <unistd.h>
    #define WRITE_FUNC write
    #define OPEN_FUNC open
    #define CLOSE_FUNC close
    #define READ_FUNC read
    #define O_FLAGS O_CREAT | O_TRUNC | O_WRONLY
    #define S_FLAGS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
#endif

#if DEBUG
#include <cpptrace/cpptrace.hpp>
#endif

namespace vex {

    struct LogEntry {
        char time[16];
        char level[8];
        char msg[256];
    };

    static std::array<LogEntry, 200> g_RingBuffer;
    static size_t g_RingHead = 0;
    static std::mutex g_RingMutex;

    static std::atomic<bool> g_IsCrashing(false);
    static int g_CrashLogFD = -1;

    static std::string get_time_str() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    static const char* get_level_str(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARNING";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    static void push_to_ring(LogLevel level, const char* msg) {
        if(g_IsCrashing) return;

        std::unique_lock<std::mutex> lock(g_RingMutex, std::defer_lock);
        if(!lock.try_lock()) return;

        LogEntry& entry = g_RingBuffer[g_RingHead];

        time_t rawtime;
        time(&rawtime);
        struct tm* timeinfo = localtime(&rawtime);
        strftime(entry.time, sizeof(entry.time), "%H:%M:%S", timeinfo);

        const char* lvl = get_level_str(level);
        strncpy(entry.level, lvl, 7); entry.level[7] = '\0';
        strncpy(entry.msg, msg, 255); entry.msg[255] = '\0';

        g_RingHead = (g_RingHead + 1) % g_RingBuffer.size();
    }

    static void SafeWriteStr(int fd, const char* s) {
        if (!s || fd < 0) return;
        size_t len = 0;
        while (s[len]) len++;
        WRITE_FUNC(fd, s, len);
    }

    static void SafeWriteHex(int fd, uintptr_t val) {
        if (fd < 0) return;
        char buf[32];
        int i = 30;
        buf[31] = '\n';

        if (val == 0) {
            SafeWriteStr(fd, "0x0\n");
            return;
        }

        while (val > 0 && i > 1) {
            uintptr_t digit = val % 16;
            buf[i] = (digit < 10) ? ('0' + digit) : ('a' + (digit - 10));
            val /= 16;
            i--;
        }
        buf[i] = 'x';
        buf[i-1] = '0';
        WRITE_FUNC(fd, &buf[i-1], 32 - (i-1));
    }

    static void DumpProcessMap(int outFd) {
        SafeWriteStr(outFd, "\n--- MEMORY MAP (Base Addresses) ---\n");
        #ifdef __linux__
        int mapFd = OPEN_FUNC("/proc/self/maps", O_RDONLY, 0);
        if (mapFd >= 0) {
            char buf[2048];
            ssize_t bytesRead;
            while ((bytesRead = READ_FUNC(mapFd, buf, sizeof(buf))) > 0) {
                WRITE_FUNC(outFd, buf, bytesRead);
            }
            CLOSE_FUNC(mapFd);
        }
        #elif defined(_WIN32)
        HANDLE hProcess = GetCurrentProcess();
        HMODULE hMods[1024];
        DWORD cbNeeded;
        if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                char szModName[MAX_PATH];
                if (GetModuleFileNameA(hMods[i], szModName, sizeof(szModName))) {
                    MODULEINFO modInfo;
                    if(GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                        SafeWriteStr(outFd, "Base: ");
                        SafeWriteHex(outFd, (uintptr_t)modInfo.lpBaseOfDll);
                        SafeWriteStr(outFd, " | ");
                        SafeWriteStr(outFd, szModName);
                        SafeWriteStr(outFd, "\n");
                    }
                }
            }
        }
        #endif
        SafeWriteStr(outFd, "-----------------------------------\n");
    }

    static void DumpRegisters(int fd, void* context) {
        if (!context) return;
        SafeWriteStr(fd, "\n--- CPU REGISTERS ---\n");
        #if defined(__linux__) && defined(__x86_64__)
            ucontext_t* uc = (ucontext_t*)context;
            SafeWriteStr(fd, "RAX: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RAX]);
            SafeWriteStr(fd, "RBX: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RBX]);
            SafeWriteStr(fd, "RCX: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RCX]);
            SafeWriteStr(fd, "RDX: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RDX]);
            SafeWriteStr(fd, "RSI: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RSI]);
            SafeWriteStr(fd, "RDI: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RDI]);
            SafeWriteStr(fd, "RBP: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RBP]);
            SafeWriteStr(fd, "RSP: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RSP]);
            SafeWriteStr(fd, "RIP: "); SafeWriteHex(fd, uc->uc_mcontext.gregs[REG_RIP]);
        #elif defined(_WIN32)
            PCONTEXT ctx = ((PEXCEPTION_POINTERS)context)->ContextRecord;
            SafeWriteStr(fd, "RAX: "); SafeWriteHex(fd, ctx->Rax);
            SafeWriteStr(fd, "RBX: "); SafeWriteHex(fd, ctx->Rbx);
            SafeWriteStr(fd, "RCX: "); SafeWriteHex(fd, ctx->Rcx);
            SafeWriteStr(fd, "RDX: "); SafeWriteHex(fd, ctx->Rdx);
            SafeWriteStr(fd, "RSI: "); SafeWriteHex(fd, ctx->Rsi);
            SafeWriteStr(fd, "RDI: "); SafeWriteHex(fd, ctx->Rdi);
            SafeWriteStr(fd, "RBP: "); SafeWriteHex(fd, ctx->Rbp);
            SafeWriteStr(fd, "RSP: "); SafeWriteHex(fd, ctx->Rsp);
            SafeWriteStr(fd, "RIP: "); SafeWriteHex(fd, ctx->Rip);
            SafeWriteStr(fd, "R8:  "); SafeWriteHex(fd, ctx->R8);
            SafeWriteStr(fd, "R9:  "); SafeWriteHex(fd, ctx->R9);
            SafeWriteStr(fd, "R10: "); SafeWriteHex(fd, ctx->R10);
            SafeWriteStr(fd, "R11: "); SafeWriteHex(fd, ctx->R11);
        #endif
    }

    static void WriteCrashReport(const char* reason, void* context) {
        bool expected = false;
        if (!g_IsCrashing.compare_exchange_strong(expected, true)) return;

        int fd = g_CrashLogFD;
        uintptr_t crashAddr = 0;

        #if defined(__linux__) && defined(__x86_64__)
        if (context) crashAddr = (uintptr_t)((ucontext_t*)context)->uc_mcontext.gregs[REG_RIP];
        #elif defined(_WIN32)
        if (context) crashAddr = (uintptr_t)((PEXCEPTION_POINTERS)context)->ExceptionRecord->ExceptionAddress;
        #endif

        if (fd >= 0) {
            SafeWriteStr(fd, "\n========== CRASH OCCURRED ==========\nReason: ");
            SafeWriteStr(fd, reason);
            SafeWriteStr(fd, "\n");

            if (crashAddr != 0) {
                SafeWriteStr(fd, "Absolute Crash Address: ");
                SafeWriteHex(fd, crashAddr);
            }

            DumpRegisters(fd, context);
            DumpProcessMap(fd);

            SafeWriteStr(fd, "\n--- RECENT LOGS ---\n");
            size_t idx = g_RingHead;
            for(size_t k=0; k<g_RingBuffer.size(); ++k) {
                const auto& e = g_RingBuffer[idx];
                if(e.msg[0] != '\0') {
                    SafeWriteStr(fd, "["); SafeWriteStr(fd, e.time); SafeWriteStr(fd, "] ");
                    SafeWriteStr(fd, "["); SafeWriteStr(fd, e.level); SafeWriteStr(fd, "] ");
                    SafeWriteStr(fd, e.msg); SafeWriteStr(fd, "\n");
                }
                idx = (idx + 1) % g_RingBuffer.size();
            }

            const char* msg = "\n[CRITICAL] CRASH DUMP SAVED TO LOG.\n";
            WRITE_FUNC(2, msg, 36);
        } else {
            const char* msg = "\n[CRITICAL] NO CRASH FILE OPEN.\n";
            WRITE_FUNC(2, msg, 31);
        }
    }

#if defined(_WIN32)
    LONG WINAPI WindowsCrashHandler(EXCEPTION_POINTERS* p) {
        WriteCrashReport("Exception (Windows)", p);
        #if DEBUG
            cpptrace::generate_trace().print();
        #endif
        return EXCEPTION_EXECUTE_HANDLER;
    }
#else
    void PosixSignalHandler(int sig, siginfo_t* info, void* context) {
        const char* name = "Unknown";
        if(sig == SIGSEGV) {
            if (info && info->si_code == SEGV_MAPERR) name = "SIGSEGV (Address not mapped)";
            else if (info && info->si_code == SEGV_ACCERR) name = "SIGSEGV (Access denied)";
            else name = "SIGSEGV";
        }
        else if(sig == SIGABRT) name = "SIGABRT";
        else if(sig == SIGILL) name = "SIGILL";
        else if(sig == SIGFPE) name = "SIGFPE";

        WriteCrashReport(name, context);

        #if DEBUG
            cpptrace::generate_trace().print();
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(sig, &sa, nullptr);
            raise(sig);
        #else
            _exit(1);
        #endif
    }
#endif

    void InitCrashHandler() {
        if(g_CrashLogFD >= 0) return;

        std::filesystem::path logPath = GetLogDir() / "game_session.log";
        std::string pathStr = logPath.string();

        g_CrashLogFD = OPEN_FUNC(pathStr.c_str(), O_FLAGS, S_FLAGS);

        if (g_CrashLogFD >= 0) {
            SafeWriteStr(g_CrashLogFD, "=== VEX ENGINE SESSION START ===\n");
            const char* msg = "[SYSTEM] Crash Handler Initialized.\n";
            WRITE_FUNC(2, msg, 32);
        } else {
            fprintf(stderr, "[ERROR] Failed to open persistent log file at: %s\n", pathStr.c_str());
        }

#if defined(_WIN32)
        SetUnhandledExceptionFilter(WindowsCrashHandler);
#else
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
        sa.sa_sigaction = PosixSignalHandler;
        sigaction(SIGSEGV, &sa, nullptr);
        sigaction(SIGABRT, &sa, nullptr);
        sigaction(SIGILL, &sa, nullptr);
        sigaction(SIGFPE, &sa, nullptr);
#endif
    }

    static void log_internal(LogLevel level, const char* fmt, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);
        int len = vsnprintf(nullptr, 0, fmt, args_copy);
        va_end(args_copy);

        if (len < 0) return;

        std::vector<char> buffer(len + 1);
        vsnprintf(buffer.data(), buffer.size(), fmt, args);

        push_to_ring(level, buffer.data());

        if (g_CrashLogFD >= 0) {
            std::string timeStr = get_time_str();
            const char* levelStr = get_level_str(level);
            SafeWriteStr(g_CrashLogFD, "["); SafeWriteStr(g_CrashLogFD, timeStr.c_str()); SafeWriteStr(g_CrashLogFD, "] ");
            SafeWriteStr(g_CrashLogFD, "["); SafeWriteStr(g_CrashLogFD, levelStr); SafeWriteStr(g_CrashLogFD, "] ");
            SafeWriteStr(g_CrashLogFD, buffer.data()); SafeWriteStr(g_CrashLogFD, "\n");
        }

        bool quiet = false;
        #if !DEBUG
            if (level == LogLevel::INFO || level == LogLevel::WARNING) quiet = true;
            #ifdef DIST_BUILD
                quiet = true;
            #endif
        #endif

        if (!quiet) {
            std::string timeStr = get_time_str();
            const char* levelStr = get_level_str(level);
            SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
                           "%s [%s] %s", timeStr.c_str(), levelStr, buffer.data());
        }
    }

#if DEBUG
    [[noreturn]] void throw_error(const std::string& msg) {
        throw cpptrace::runtime_error(msg.c_str());
    }
    void log(const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        log_internal(LogLevel::INFO, fmt, args); va_end(args);
    }
    void log(LogLevel level, const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        log_internal(level, fmt, args); va_end(args);
    }
    void handle_exception(const std::exception& e) {
        if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
            cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
        } else {
            log(LogLevel::CRITICAL, "%s", e.what());
        }
    }
    void handle_critical_exception(const std::exception& e) {
        if(auto* cpptr = dynamic_cast<const cpptrace::exception*>(&e)) {
            cpptr->trace().print(std::cerr, cpptrace::isatty(cpptrace::stderr_fileno));
        } else {
            log(LogLevel::CRITICAL, "%s", e.what());
        }
        throw;
    }
#else
    [[noreturn]] void throw_error(const std::string& msg) {
        WriteCrashReport(msg.c_str(), nullptr);
        if (g_CrashLogFD >= 0) CLOSE_FUNC(g_CrashLogFD);
        _exit(1);
    }
    void log(const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        log_internal(LogLevel::INFO, fmt, args); va_end(args);
    }
    void log(LogLevel level, const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        log_internal(level, fmt, args); va_end(args);
    }
    void handle_exception(const std::exception& e) {
        push_to_ring(LogLevel::ERROR, e.what());
        #ifndef DIST_BUILD
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        #endif
    }
    void handle_critical_exception(const std::exception& e) {
        WriteCrashReport(e.what(), nullptr);
        if (g_CrashLogFD >= 0) CLOSE_FUNC(g_CrashLogFD);
        #ifndef DIST_BUILD
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "%s", e.what());
        #endif
        _exit(1);
    }
#endif

}
