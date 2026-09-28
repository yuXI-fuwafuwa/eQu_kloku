#include "progress_store.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <stdexcept>
#include <system_error>

#include <unistd.h>

namespace {
namespace fs = std::filesystem;

fs::path ProgressPath()
{
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg && fs::path(xdg).is_absolute()) {
        return fs::path(xdg) / "equ-kloku" / "progress.txt";
    }

    const char* home = std::getenv("HOME");
    if (!home || !*home) throw std::runtime_error("HOME is not set");
    return fs::path(home) / ".local" / "share" / "equ-kloku" / "progress.txt";
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
        temporary += ".tmp." + std::to_string(getpid());

        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            if (!file) throw std::runtime_error("Cannot open temporary progress file");
            file << count << '\n';
            file.flush();
            if (!file) throw std::runtime_error("Cannot write temporary progress file");
        }
        fs::rename(temporary, path);
        return {};
    } catch (const std::exception&) {
        if (!temporary.empty()) {
            std::error_code ignored;
            fs::remove(temporary, ignored);
        }
        return "完成了，但暂时无法保存次数";
    }
}
