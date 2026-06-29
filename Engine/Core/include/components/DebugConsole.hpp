#pragma once
#include "ErrorUtils.hpp"
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <mutex>

#if DEBUG
#include <imgui.h>
#endif

namespace vex {
    class VEX_EXPORT DebugConsole {
    public:
        static DebugConsole& Get() {
            static DebugConsole instance;
            return instance;
        }

        #if DEBUG
        /// @brief The static callback ImGui will call
        static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

        /// @brief The internal handler
        int TextEditCallback(ImGuiInputTextCallbackData* data);
        #endif

        /// @brief Initialize the debug console.
        void Init();

        /// @brief Draw the debug console.
        /// @param p_open Pointer to a boolean indicating whether the console is open.
        /// @param isEditorMode Indicates whether the console is in editor mode.
        void Draw(bool* p_open, bool isEditorMode);

        using CommandFunc = std::function<void(const std::vector<std::string>&)>;

        /// @brief Register a command with the debug console.
        /// @param name The name of the command.
        /// @param cmd The function to execute when the command is called.
        void RegisterCommand(const std::string& name, CommandFunc cmd);

        /// @brief Execute a command with the given arguments.
        /// @param commandLine The command line to execute.
        void Execute(const std::string& commandLine);

        /// @brief API for the callback
        /// @param level The log level.
        /// @param msg The log message.
        void AddLog(LogLevel level, const char* msg);

    private:
        DebugConsole() = default;

        struct LogEntry {
            std::string text;
            LogLevel level;
        };

        std::vector<LogEntry> m_logs;
        std::map<std::string, CommandFunc> m_commands;
        std::vector<std::string> m_history;
        int m_historyPos = -1; // -1: new line, 0..History.Size-1 browsing history

        char m_inputBuf[256] = {0};
        bool m_scrollToBottom = true;
        std::mutex m_mutex;
    };
}
