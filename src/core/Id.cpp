#include "core/Id.h"
#include <cstdio>
#include <mutex>
#include <random>
namespace at {
std::string newId() {
    static std::mt19937_64 rng{std::random_device{}() ^ 0x9E3779B97F4A7C15ULL};
    static std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", static_cast<unsigned long long>(rng()));
    return buf;
}
}
