#include "components/DebugConsole.hpp"
#include <sstream>

namespace vex {
    static std::vector<std::string> splitString(const std::string& str) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (ss >> token) tokens.push_back(token);
        return tokens;
    }

    void DebugConsole::Init() {
        RegisterCommand("clear", [this](auto args){
            std::lock_guard<std::mutex> lock(m_mutex);
            m_logs.clear();
        });

        RegisterCommand("help", [this](auto args){
            log(LogLevel::INFO, "Available commands:");
            for(const auto& [name, func] : m_commands) {
                log(LogLevel::INFO, " - %s", name.c_str());
            }
        });

        AddLogCallback([](LogLevel level, const char* msg) {
            DebugConsole::Get().AddLog(level, msg);
        });

        log("Console System Initialized.");
    }

    void DebugConsole::RegisterCommand(const std::string& name, CommandFunc cmd) {
        m_commands[name] = cmd;
    }

    void DebugConsole::Execute(const std::string& commandLine) {
        log(LogLevel::INFO, "# %s", commandLine.c_str());

        m_history.push_back(commandLine);
        m_historyPos = -1;

        auto tokens = splitString(commandLine);
        if (tokens.empty()) return;

        std::string cmdName = tokens[0];
        if (m_commands.find(cmdName) != m_commands.end()) {
            tokens.erase(tokens.begin());
            m_commands[cmdName](tokens);
        } else {
            log(LogLevel::ERROR, "Unknown command: '%s'", cmdName.c_str());
        }
    }

    void DebugConsole::AddLog(LogLevel level, const char* msg) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.push_back({ std::string(msg), level });
        m_scrollToBottom = true;
    }

    #if DEBUG
    int DebugConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data) {
        DebugConsole* console = (DebugConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    int DebugConsole::TextEditCallback(ImGuiInputTextCallbackData* data) {
        switch (data->EventFlag) {
            case ImGuiInputTextFlags_CallbackHistory: {
                const int prevHistoryPos = m_historyPos;
                if (data->EventKey == ImGuiKey_UpArrow) {
                    if (m_historyPos == -1)
                        m_historyPos = m_history.size() - 1;
                    else if (m_historyPos > 0)
                        m_historyPos--;
                } else if (data->EventKey == ImGuiKey_DownArrow) {
                    if (m_historyPos != -1)
                        if (++m_historyPos >= m_history.size())
                            m_historyPos = -1;
                }

                if (prevHistoryPos != m_historyPos) {
                    const char* historyStr = (m_historyPos >= 0) ? m_history[m_historyPos].c_str() : "";
                    data->DeleteChars(0, data->BufTextLen);
                    data->InsertChars(0, historyStr);
                }
            }
        }
        return 0;
    }
    #endif

    void DebugConsole::Draw(bool* p_open, bool isEditorMode) {
        #if DEBUG
        if (!*p_open) return;

        if (!isEditorMode) {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 350));
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        }

        ImGuiWindowFlags flags = isEditorMode ? 0 : (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

        if (ImGui::Begin("Console", p_open, flags)) {

            const float footerHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& item : m_logs) {
                    ImVec4 color = ImVec4(1,1,1,1);
                    if (item.level == LogLevel::ERROR || item.level == LogLevel::CRITICAL) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                    else if (item.level == LogLevel::WARNING) color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(item.text.c_str());
                    ImGui::PopStyleColor();
                }
            }

            if (m_scrollToBottom || (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
                ImGui::SetScrollHereY(1.0f);
            m_scrollToBottom = false;

            ImGui::EndChild();
            ImGui::Separator();
            bool reclaimFocus = false;

            ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue |
                                             ImGuiInputTextFlags_CallbackHistory |
                                             ImGuiInputTextFlags_CallbackCompletion;

            if (ImGui::InputText("##ConsoleInput", m_inputBuf, IM_ARRAYSIZE(m_inputBuf), inputFlags, &TextEditCallbackStub, (void*)this)) {
                Execute(m_inputBuf);
                strcpy(m_inputBuf, "");
                reclaimFocus = true;
            }

            ImGui::SetItemDefaultFocus();
            if (reclaimFocus || (!isEditorMode && ImGui::IsWindowAppearing()))
                ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();

        if (!isEditorMode) ImGui::PopStyleVar();
        #else
        return;
        #endif
    }
}
