#include "media/MediaManager.h"
#include <filesystem>
#include "core/FileUtil.h"
#include "core/Id.h"
namespace at {

const Frame* MediaManager::frame(const std::string& key, const std::string& path, double t) {
    Entry& e = entries_[key];
    if (e.path != path) {          // new layer, or footage was relinked
        e.path = path;
        e.reader.reset();
        auto r = std::make_unique<VideoReader>();
        std::string err;
        if (r->open(path, err)) e.reader = std::move(r);
    }
    return e.reader ? e.reader->frameAt(t) : nullptr;
}

void MediaManager::clear() { entries_.clear(); }

bool MediaManager::probe(const std::string& path, Footage& out, std::string& err) {
    MediaInfo mi;
    if (!probeMedia(path, mi, err)) return false;
    out = Footage{};
    out.id = newId();
    out.path = path;
    {
        const auto u8 = pathFromUtf8(path).filename().u8string();
        out.name = std::string(u8.begin(), u8.end());
        if (out.name.empty()) out.name = path;
    }
    out.hasAudio = mi.hasAudio;
    if (mi.hasVideo) {
        out.kind = mi.isImage ? FootageKind::Image : FootageKind::Video;
        out.width = mi.width; out.height = mi.height; out.fps = mi.fps; out.duration = mi.duration;
    } else {
        out.kind = FootageKind::Audio;
        out.duration = mi.duration;
    }
    return true;
}

bool MediaManager::grabPreviewFrame(const std::string& path, Frame& out) {
    VideoReader r;
    std::string err;
    if (!r.open(path, err)) return false;
    const double t = r.info().isImage ? 0.0 : r.info().duration * 0.1;
    const Frame* f = r.frameAt(t);
    if (!f) return false;
    out = *f;
    return true;
}
}
