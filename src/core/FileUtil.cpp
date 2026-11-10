#include "core/FileUtil.h"
#include <fstream>
#include <sstream>
namespace at {
bool readTextFile(const std::string& p, std::string& out, std::string& err) {
    std::ifstream f(pathFromUtf8(p), std::ios::binary);
    if (!f) { err = "Cannot open file for reading: " + p; return false; }
    std::ostringstream ss; ss << f.rdbuf();
    out = ss.str();
    return true;
}
bool writeTextFileAtomic(const std::string& p, const std::string& text, std::string& err) {
    namespace fs = std::filesystem;
    fs::path target = pathFromUtf8(p);
    fs::path tmp = target; tmp += ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) { err = "Cannot open file for writing: " + p; return false; }
        f.write(text.data(), static_cast<std::streamsize>(text.size()));
        f.flush();
        if (!f) { err = "Write failed (disk full?): " + p; return false; }
    }
    std::error_code ec;
    fs::rename(tmp, target, ec);
    if (ec) { err = "Cannot replace file: " + ec.message(); fs::remove(tmp, ec); return false; }
    return true;
}
bool fileExists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(pathFromUtf8(p), ec);
}
}
