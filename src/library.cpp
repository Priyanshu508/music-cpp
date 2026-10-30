#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
//func to check if the file has files of given extensions
bool has_audio_extension(const fs::path& p) {
    std::string ext = p.extension().string(); // get only the extension part of file then convert it to string
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext == ".mp3" || ext == ".flac" || ext == ".wav" ||
           ext == ".m4a" || ext == ".m4b" || ext == ".aac" ||
           ext == ".opus" || ext == ".ogg" || ext == ".oga" ||
           ext == ".webm" || ext == ".mp4" || ext == ".mka" ||
           ext == ".mkv" || ext == ".wma" || ext == ".ape" ||
           ext == ".wv" || ext == ".mpc" || ext == ".aif" || ext == ".aiff";
}

} // namespace

std::vector<fs::path> scan_directory(const fs::path& dir) {
    std::vector<fs::path> results;

    std::error_code ec;
    fs::recursive_directory_iterator it(
        dir, fs::directory_options::skip_permission_denied, ec);

    for (const auto& entry : it) {
        if (entry.is_regular_file(ec) && has_audio_extension(entry.path())) {
            results.push_back(entry.path());
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}
