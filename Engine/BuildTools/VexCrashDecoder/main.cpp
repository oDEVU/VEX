#include "CrashDecoder.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: VexCrashDecoder <game_session.log> <symbols_folder> [binaries_folder]\n";
        std::cerr << "  binaries_folder is optional - if provided, the tool will try to load the matching DLL/EXE from there for better symbol resolution.\n";
        std::cerr << "Example: VexCrashDecoder crash.log \"Build/Symbols/Distribution/2025-12-30_03-00/\" \"Build/Distribution/Binaries/\"\n";
        return 1;
    }

    std::string logPath = argv[1];
    std::string symbolsFolder = argv[2];
    std::string binariesFolder = (argc == 4) ? argv[3] : "";

    CrashDecoder decoder;

    if (!decoder.loadLog(logPath)) {
        std::cerr << "Failed to parse crash log: " << logPath << "\n";
        return 1;
    }

    if (!decoder.loadSymbolsFromFolder(symbolsFolder)) {
        std::cerr << "No debug symbols found in: " << symbolsFolder << "\n";
        return 1;
    }

    // Pass binaries folder to decoder (new method)
    decoder.setBinariesFolder(binariesFolder);

    std::string report = decoder.decode();
    std::cout << report << std::endl;

    return 0;
}
