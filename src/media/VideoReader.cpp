#include "media/VideoReader.h"
#include <algorithm>
#include <cmath>
#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace at {
namespace {
std::string ffErr(int code) {
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(code, buf, sizeof buf);
    return buf;
}
bool looksLikeImageFormat(const AVInputFormat* f) {
    if (!f || !f->name) return false;
    const std::string n = f->name;
    return n.find("_pipe") != std::string::npos || n == "image2" || n == "image2pipe";
}
// Fills 'info' from an already opened format context. Returns the video stream index (or -1).
int fillInfo(AVFormatContext* fmt, MediaInfo& info) {
    info = MediaInfo{};
    int v = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (v >= 0 && (fmt->streams[v]->disposition & AV_DISPOSITION_ATTACHED_PIC)) v = -1;   // cover art in mp3
    info.hasAudio = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0) >= 0;
    if (fmt->duration > 0) info.duration = static_cast<double>(fmt->duration) / AV_TIME_BASE;
    if (v >= 0) {
        AVStream* st = fmt->streams[v];
        info.hasVideo = true;
        info.width = st->codecpar->width;
        info.height = st->codecpar->height;
        AVRational r = av_guess_frame_rate(fmt, st, nullptr);
        info.fps = (r.num > 0 && r.den > 0) ? av_q2d(r) : 0.0;
        info.isImage = looksLikeImageFormat(fmt->iformat);
        if (info.duration <= 0 && st->duration > 0) info.duration = st->duration * av_q2d(st->time_base);
        if (info.isImage) { info.duration = 0; if (info.fps <= 0) info.fps = 30; }
        else if (info.fps <= 0) info.fps = 30;
    }
    return v;
}
}  // namespace

bool probeMedia(const std::string& path, MediaInfo& info, std::string& err) {
    AVFormatContext* fmt = nullptr;
    int r = avformat_open_input(&fmt, path.c_str(), nullptr, nullptr);
    if (r < 0) { err = "Cannot open file: " + ffErr(r); return false; }
    r = avformat_find_stream_info(fmt, nullptr);
    if (r < 0) { err = "Cannot read stream info: " + ffErr(r); avformat_close_input(&fmt); return false; }
    fillInfo(fmt, info);
    avformat_close_input(&fmt);
    if (!info.hasVideo && !info.hasAudio) { err = "No audio or video found in this file."; return false; }
    return true;
}

struct VideoReader::Impl {
    AVFormatContext* fmt = nullptr;
    AVCodecContext* dec = nullptr;
    AVFrame* frame = nullptr;     // scratch frame for decoding
    AVFrame* held = nullptr;      // last decoded frame we want to keep
    AVPacket* pkt = nullptr;
    SwsContext* sws = nullptr;
    int vstream = -1;
    double timeBase = 0, startSec = 0, frameDur = 1.0 / 30;
    bool eof = false;
    bool haveCache = false;
    double cacheTime = 0;         // composition-independent source time of 'cache'
    double lastPtsSec = 0;
    MediaInfo info;
    Frame cache;

    ~Impl() { close(); }
    void close() {
        if (sws) { sws_freeContext(sws); sws = nullptr; }
        if (pkt) av_packet_free(&pkt);
        if (held) av_frame_free(&held);
        if (frame) av_frame_free(&frame);
        if (dec) avcodec_free_context(&dec);
        if (fmt) avformat_close_input(&fmt);
        haveCache = false;
    }

    // Decodes the next frame into 'frame'. Returns false at end of stream.
    bool decodeNext() {
        while (true) {
            int r = avcodec_receive_frame(dec, frame);
            if (r == 0) return true;
            if (r != AVERROR(EAGAIN)) return false;   // EOF or real error
            if (eof) return false;
            while (true) {
                r = av_read_frame(fmt, pkt);
                if (r < 0) { eof = true; avcodec_send_packet(dec, nullptr); break; }
                if (pkt->stream_index == vstream) {
                    avcodec_send_packet(dec, pkt);
                    av_packet_unref(pkt);
                    break;
                }
                av_packet_unref(pkt);
            }
        }
    }
    double frameTimeSec(const AVFrame* f) {
        int64_t pts = f->best_effort_timestamp;
        if (pts == AV_NOPTS_VALUE) pts = f->pts;
        if (pts == AV_NOPTS_VALUE) return lastPtsSec + frameDur;
        return pts * timeBase - startSec;
    }
    bool convertHeld() {
        const int w = held->width, h = held->height;
        if (w <= 0 || h <= 0) return false;
        sws = sws_getCachedContext(sws, w, h, static_cast<AVPixelFormat>(held->format), w, h,
                                   AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!sws) return false;
        // Respect limited/full range and HD vs SD colour matrix for YUV sources.
        const int srcRange = held->color_range == AVCOL_RANGE_JPEG ? 1 : 0;
        int cs = SWS_CS_DEFAULT;
        if (held->colorspace == AVCOL_SPC_BT709) cs = SWS_CS_ITU709;
        else if (held->colorspace == AVCOL_SPC_BT2020_NCL || held->colorspace == AVCOL_SPC_BT2020_CL) cs = SWS_CS_BT2020;
        else if (held->colorspace == AVCOL_SPC_UNSPECIFIED && h >= 720) cs = SWS_CS_ITU709;
        sws_setColorspaceDetails(sws, sws_getCoefficients(cs), srcRange, sws_getCoefficients(SWS_CS_DEFAULT), 1, 0, 1 << 16, 1 << 16);
        cache.allocate(w, h);
        uint8_t* dst[4] = {cache.rgba.data(), nullptr, nullptr, nullptr};
        int dstStride[4] = {w * 4, 0, 0, 0};
        sws_scale(sws, held->data, held->linesize, 0, h, dst, dstStride);
        return true;
    }
};

VideoReader::VideoReader() : d(new Impl) {}
VideoReader::~VideoReader() = default;
bool VideoReader::isOpen() const { return d->fmt != nullptr; }
const MediaInfo& VideoReader::info() const { return d->info; }

bool VideoReader::open(const std::string& path, std::string& err) {
    d->close();
    d->eof = false; d->haveCache = false;
    int r = avformat_open_input(&d->fmt, path.c_str(), nullptr, nullptr);
    if (r < 0) { err = "Cannot open file: " + ffErr(r); d->fmt = nullptr; return false; }
    r = avformat_find_stream_info(d->fmt, nullptr);
    if (r < 0) { err = "Cannot read stream info: " + ffErr(r); d->close(); return false; }
    d->vstream = fillInfo(d->fmt, d->info);
    if (d->vstream < 0) { err = "File has no picture stream."; d->close(); return false; }
    AVStream* st = d->fmt->streams[d->vstream];
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!codec) { err = "No decoder available for this video format."; d->close(); return false; }
    d->dec = avcodec_alloc_context3(codec);
    if (!d->dec) { err = "Out of memory."; d->close(); return false; }
    avcodec_parameters_to_context(d->dec, st->codecpar);
    d->dec->thread_count = 0;   // let FFmpeg pick
    d->dec->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;
    r = avcodec_open2(d->dec, codec, nullptr);
    if (r < 0) { err = "Cannot start decoder: " + ffErr(r); d->close(); return false; }
    d->frame = av_frame_alloc();
    d->held = av_frame_alloc();
    d->pkt = av_packet_alloc();
    d->timeBase = av_q2d(st->time_base);
    if (st->start_time != AV_NOPTS_VALUE) d->startSec = st->start_time * d->timeBase;
    else if (d->fmt->start_time != AV_NOPTS_VALUE) d->startSec = static_cast<double>(d->fmt->start_time) / AV_TIME_BASE;
    d->frameDur = 1.0 / std::max(1.0, d->info.fps);
    return true;
}

const Frame* VideoReader::frameAt(double t) {
    Impl& s = *d;
    if (!s.fmt) return nullptr;

    if (s.info.isImage) {   // stills: decode once, serve forever
        if (!s.haveCache) {
            if (!s.decodeNext()) return nullptr;
            av_frame_unref(s.held);
            av_frame_ref(s.held, s.frame);
            av_frame_unref(s.frame);
            if (!s.convertHeld()) return nullptr;
            s.haveCache = true;
        }
        return &s.cache;
    }

    t = std::max(0.0, t);
    if (s.info.duration > 0) t = std::min(t, std::max(0.0, s.info.duration - s.frameDur * 0.5));
    const double fd = s.frameDur;

    if (s.haveCache && t >= s.cacheTime - 1e-4 && t < s.cacheTime + fd - 1e-4) return &s.cache;

    const bool needSeek = !s.haveCache || t < s.cacheTime - 1e-4 || t > s.cacheTime + 1.0;
    if (needSeek) {
        const int64_t ts = static_cast<int64_t>((t + s.startSec) / s.timeBase);
        av_seek_frame(s.fmt, s.vstream, ts, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(s.dec);
        s.eof = false;
    }

    bool got = false;
    double gotTime = 0;
    while (s.decodeNext()) {
        const double ft = s.frameTimeSec(s.frame);
        s.lastPtsSec = ft;
        av_frame_unref(s.held);
        av_frame_ref(s.held, s.frame);
        av_frame_unref(s.frame);
        got = true; gotTime = ft;
        if (ft + fd > t - 1e-4) break;   // this frame covers time t
    }
    if (!got) return s.haveCache ? &s.cache : nullptr;
    if (!s.convertHeld()) return s.haveCache ? &s.cache : nullptr;
    s.cacheTime = gotTime;
    s.haveCache = true;
    return &s.cache;
}
}
