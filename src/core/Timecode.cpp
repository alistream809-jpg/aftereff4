#include "core/Timecode.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
namespace at {
double snapToFrame(double seconds, double fps) {
    if (fps <= 0) return seconds;
    return std::round(seconds * fps) / fps;
}
std::string formatTimecode(double seconds, double fps) {
    if (fps <= 0) fps = 30;
    const bool neg = seconds < 0;
    long long frames = std::llround(std::fabs(seconds) * fps);
    const long long fpsInt = std::max<long long>(1, std::llround(fps));
    const long long ff = frames % fpsInt;
    long long total = frames / fpsInt;
    const long long ss = total % 60; total /= 60;
    const long long mm = total % 60;
    const long long hh = total / 60;
    char buf[48];
    std::snprintf(buf, sizeof buf, "%s%02lld:%02lld:%02lld:%02lld", neg ? "-" : "", hh, mm, ss, ff);
    return buf;
}
}
