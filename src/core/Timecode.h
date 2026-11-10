#pragma once
#include <string>
namespace at {
std::string formatTimecode(double seconds, double fps);   // HH:MM:SS:FF
double snapToFrame(double seconds, double fps);           // nearest frame boundary
}
