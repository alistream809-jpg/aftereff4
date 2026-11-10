#pragma once
#include <memory>
#include <string>
#include "core/Frame.h"
namespace at {

struct MediaInfo {
    bool hasVideo = false, hasAudio = false, isImage = false;
    int width = 0, height = 0;
    double fps = 0, duration = 0;   // seconds
};

// Reads container information with FFmpeg. Returns false and fills 'err' on failure.
bool probeMedia(const std::string& utf8Path, MediaInfo& info, std::string& err);

// Random-access video/image decoder (FFmpeg). One instance = one open file.
class VideoReader {
public:
    VideoReader();
    ~VideoReader();
    VideoReader(const VideoReader&) = delete;
    VideoReader& operator=(const VideoReader&) = delete;

    bool open(const std::string& utf8Path, std::string& err);
    bool isOpen() const;
    const MediaInfo& info() const;
    // Returns the picture that is visible at time t (seconds). Valid until the next call.
    const Frame* frameAt(double t);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
}
