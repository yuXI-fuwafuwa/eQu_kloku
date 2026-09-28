#pragma once

#include <cstdint>
#include <string>

struct ProgressResult {
    std::uint64_t count = 0;
    std::string error;
};

// Successful sessions are user data, so they live in XDG_DATA_HOME rather than a cache.
ProgressResult LoadProgress();
std::string SaveProgress(std::uint64_t count);
