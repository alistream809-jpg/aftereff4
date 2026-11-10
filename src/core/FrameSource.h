#pragma once
#include <string>
#include "core/Frame.h"
namespace at {
// Anything that can deliver decoded pictures. The compositor only knows this
// interface, so it never depends on FFmpeg (and can be unit-tested).
class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    // key: unique per layer (lets each layer keep its own decoder position).
    // Returns nullptr when the media cannot be read. The pointer stays valid
    // until the next call on the same key.
    virtual const Frame* frame(const std::string& key, const std::string& path, double sourceTime) = 0;
};
}
