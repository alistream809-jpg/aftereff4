#pragma once
#include <string>
#include <filesystem>
namespace at {
inline std::filesystem::path pathFromUtf8(const std::string& s) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
}
bool readTextFile(const std::string& utf8Path, std::string& out, std::string& err);
// Writes to "<path>.tmp" first and then renames, so a crash never leaves a half-written project.
bool writeTextFileAtomic(const std::string& utf8Path, const std::string& text, std::string& err);
bool fileExists(const std::string& utf8Path);
}
