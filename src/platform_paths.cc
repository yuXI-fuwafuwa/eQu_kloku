#include "platform_paths.h"

#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include "raylib.h"
#endif

std::filesystem::path ExecutableDirectory()
{
#ifdef _WIN32
    std::wstring buffer(512, L'\0');
    for (;;) {
        const DWORD size = GetModuleFileNameW(nullptr, buffer.data(),
                                              static_cast<DWORD>(buffer.size()));
        if (size == 0) throw std::runtime_error("Cannot locate executable");
        if (size < buffer.size()) {
            buffer.resize(size);
            return std::filesystem::path(buffer).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    std::filesystem::path directory(GetApplicationDirectory());
    if (directory.filename().empty()) directory = directory.parent_path();
    return directory;
#endif
}
