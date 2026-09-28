#include "progress_store.h"

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
namespace fs = std::filesystem;

#ifdef _WIN32
std::wstring WindowsEnvironment(const wchar_t* name)
{
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) return {};
    std::wstring value(required, L'\0');
    const DWORD copied = GetEnvironmentVariableW(name, value.data(), required);
    if (copied == 0 || copied >= required) return {};
    value.resize(copied);
    return value;
}
#endif

fs::path ProgressPath()
{
#ifdef _WIN32
    const std::wstring appData = WindowsEnvironment(L"APPDATA");
    if (!appData.empty() && fs::path(appData).is_absolute()) {
        return fs::path(appData) / "equ-kloku" / "progress.txt";
    }
    const std::wstring userProfile = WindowsEnvironment(L"USERPROFILE");
    if (userProfile.empty()) throw std::runtime_error("User profile is unavailable");
    return fs::path(userProfile) / "AppData" / "Roaming" / "equ-kloku" / "progress.txt";
#else
    const char* home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is not set");
#ifdef __APPLE__
    return fs::path(home) / "Library" / "Application Support" / "equ-kloku" / "progress.txt";
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg && fs::path(xdg).is_absolute()) {
        return fs::path(xdg) / "equ-kloku" / "progress.txt";
    }
    return fs::path(home) / ".local" / "share" / "equ-kloku" / "progress.txt";
#endif
#endif
}
}

ProgressResult LoadProgress()
{
    try {
        const fs::path path = ProgressPath();
        std::ifstream file(path);
        if (!file) {
            if (!fs::exists(path)) return {};
            return {0, "无法读取已完成次数"};
        }

        std::string content;
        std::getline(file, content);
        if (!file && !file.eof()) return {0, "无法读取已完成次数"};
        if (content.empty()) return {0, "已完成次数记录格式有误"};
        for (unsigned char digit : content) {
            if (!std::isdigit(digit)) return {0, "已完成次数记录格式有误"};
        }
        file >> std::ws;
        if (!file.eof()) return {0, "已完成次数记录格式有误"};
        try {
            return {std::stoull(content), {}};
        } catch (const std::exception&) {
            return {0, "已完成次数记录格式有误"};
        }
    } catch (const std::exception&) {
        return {0, "无法读取已完成次数"};
    }
}

std::string SaveProgress(std::uint64_t count)
{
    fs::path temporary;
    try {
        const fs::path path = ProgressPath();
        fs::create_directories(path.parent_path());
        temporary = path;
        std::random_device random;
        const auto nonce = (static_cast<std::uint64_t>(random()) << 32) | random();
        temporary += ".tmp." + std::to_string(nonce);

        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            if (!file) throw std::runtime_error("Cannot open temporary progress file");
            file << count << '\n';
            file.flush();
            if (!file) throw std::runtime_error("Cannot write temporary progress file");
        }
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw std::runtime_error("Cannot replace progress file");
        }
#else
        fs::rename(temporary, path);
#endif
        return {};
    } catch (const std::exception&) {
        if (!temporary.empty()) {
            std::error_code ignored;
            fs::remove(temporary, ignored);
        }
        return "完成了，但暂时无法保存次数";
    }
}
