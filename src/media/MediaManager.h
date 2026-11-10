#pragma once
#include <map>
#include <memory>
#include <string>
#include "core/FrameSource.h"
#include "media/VideoReader.h"
#include "project/Project.h"
namespace at {

// Owns one decoder per layer and serves frames to the compositor.
class MediaManager : public IFrameSource {
public:
    const Frame* frame(const std::string& key, const std::string& path, double sourceTime) override;
    void clear();                         // drop all open files (e.g. after relink / open project)

    // Reads metadata and fills a Footage entry (id, name, path, kind, size, fps, duration).
    static bool probe(const std::string& utf8Path, Footage& out, std::string& err);
    // Decodes one representative picture (for project-panel thumbnails).
    static bool grabPreviewFrame(const std::string& utf8Path, Frame& out);

private:
    struct Entry {
        std::string path;
        std::unique_ptr<VideoReader> reader;   // null if the file could not be opened
    };
    std::map<std::string, Entry> entries_;
};
}
