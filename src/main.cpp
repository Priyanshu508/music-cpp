#include "library.h"
#include "ui.h"
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: music-cpp <music-dir>\n";
        return 1;
    }
    try {
        auto songs = scan_directory(argv[1]);
        if (songs.empty()) {
            std::cerr << "no audio files found\n";
            return 1;
        }
        run_ui(songs);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "scan failed: " << e.what() << '\n';
        return 1;
    }
}
