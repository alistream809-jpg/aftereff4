#include "comp/Compositor.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <thread>
#include <vector>

namespace at {
namespace {

void parallelRows(int y0, int y1, const std::function<void(int, int)>& fn) {
    const int rows = y1 - y0;
    if (rows <= 0) return;
    unsigned n = std::max(1u, std::thread::hardware_concurrency());
    n = std::min<unsigned>(n, static_cast<unsigned>(std::max(1, rows / 16)));
    if (n <= 1) { fn(y0, y1); return; }
    std::vector<std::thread> ts;
    const int per = (rows + static_cast<int>(n) - 1) / static_cast<int>(n);
    for (unsigned t = 0; t < n; ++t) {
        const int a = y0 + static_cast<int>(t) * per;
        const int b = std::min(y1, a + per);
        if (a >= b) break;
        ts.emplace_back(fn, a, b);
    }
    for (auto& t : ts) t.join();
}

struct Xform { double ax, ay, px, py, sx, sy, c, s; };

void drawLayer(Frame& out, const Xform& X, double opacity, double osx, double osy,
               int sw, int sh, const Frame* src, const uint8_t solid[3]) {
    if (std::fabs(X.sx) < 1e-9 || std::fabs(X.sy) < 1e-9 || opacity <= 0) return;
    // Output-space bounding box of the transformed source rectangle.
    const double cx[4] = {0, static_cast<double>(sw), static_cast<double>(sw), 0};
    const double cy[4] = {0, 0, static_cast<double>(sh), static_cast<double>(sh)};
    double minx = 1e30, miny = 1e30, maxx = -1e30, maxy = -1e30;
    for (int i = 0; i < 4; ++i) {
        const double lx = (cx[i] - X.ax) * X.sx, ly = (cy[i] - X.ay) * X.sy;
        const double ox = (X.px + X.c * lx - X.s * ly) * osx;
        const double oy = (X.py + X.s * lx + X.c * ly) * osy;
        minx = std::min(minx, ox); maxx = std::max(maxx, ox);
        miny = std::min(miny, oy); maxy = std::max(maxy, oy);
    }
    const int x0 = std::max(0, static_cast<int>(std::floor(minx)));
    const int x1 = std::min(out.width, static_cast<int>(std::ceil(maxx)) + 1);
    const int y0 = std::max(0, static_cast<int>(std::floor(miny)));
    const int y1 = std::min(out.height, static_cast<int>(std::ceil(maxy)) + 1);
    if (x0 >= x1 || y0 >= y1) return;

    parallelRows(y0, y1, [&](int ya, int yb) {
        for (int y = ya; y < yb; ++y) {
            uint8_t* row = &out.rgba[static_cast<size_t>(y) * out.width * 4];
            for (int x = x0; x < x1; ++x) {
                const double dx = (x + 0.5) / osx - X.px;
                const double dy = (y + 0.5) / osy - X.py;
                const double lx = X.c * dx + X.s * dy;
                const double ly = -X.s * dx + X.c * dy;
                const double u = X.ax + lx / X.sx;
                const double v = X.ay + ly / X.sy;
                if (u < 0 || v < 0 || u >= sw || v >= sh) continue;

                float r, g, b, a;
                if (src) {
                    const double fx = u - 0.5, fy = v - 0.5;
                    int ix = static_cast<int>(std::floor(fx)), iy = static_cast<int>(std::floor(fy));
                    const float tx = static_cast<float>(fx - ix), ty = static_cast<float>(fy - iy);
                    const int xa = std::clamp(ix, 0, src->width - 1), xb = std::clamp(ix + 1, 0, src->width - 1);
                    const int ya2 = std::clamp(iy, 0, src->height - 1), yb2 = std::clamp(iy + 1, 0, src->height - 1);
                    const uint8_t* p00 = &src->rgba[(static_cast<size_t>(ya2) * src->width + xa) * 4];
                    const uint8_t* p10 = &src->rgba[(static_cast<size_t>(ya2) * src->width + xb) * 4];
                    const uint8_t* p01 = &src->rgba[(static_cast<size_t>(yb2) * src->width + xa) * 4];
                    const uint8_t* p11 = &src->rgba[(static_cast<size_t>(yb2) * src->width + xb) * 4];
                    const float w00 = (1 - tx) * (1 - ty), w10 = tx * (1 - ty), w01 = (1 - tx) * ty, w11 = tx * ty;
                    // interpolate premultiplied to avoid dark/bright fringes at transparent edges
                    a = w00 * p00[3] + w10 * p10[3] + w01 * p01[3] + w11 * p11[3];
                    if (a <= 0.0f) continue;
                    r = (w00 * p00[0] * p00[3] + w10 * p10[0] * p10[3] + w01 * p01[0] * p01[3] + w11 * p11[0] * p11[3]) / a;
                    g = (w00 * p00[1] * p00[3] + w10 * p10[1] * p10[3] + w01 * p01[1] * p01[3] + w11 * p11[1] * p11[3]) / a;
                    b = (w00 * p00[2] * p00[3] + w10 * p10[2] * p10[3] + w01 * p01[2] * p01[3] + w11 * p11[2] * p11[3]) / a;
                } else {
                    r = solid[0]; g = solid[1]; b = solid[2]; a = 255.0f;
                }
                const float al = static_cast<float>(a / 255.0 * opacity);
                uint8_t* d = row + static_cast<size_t>(x) * 4;
                d[0] = static_cast<uint8_t>(d[0] * (1 - al) + r * al + 0.5f);
                d[1] = static_cast<uint8_t>(d[1] * (1 - al) + g * al + 0.5f);
                d[2] = static_cast<uint8_t>(d[2] * (1 - al) + b * al + 0.5f);
                d[3] = 255;
            }
        }
    });
}
}  // namespace

void renderComposition(const Project& project, const Composition& comp, double time,
                       IFrameSource& source, Frame& out, int divisor) {
    divisor = std::max(1, divisor);
    const int ow = std::max(1, comp.width / divisor);
    const int oh = std::max(1, comp.height / divisor);
    out.allocate(ow, oh);
    for (size_t i = 0; i < out.rgba.size(); i += 4) {
        out.rgba[i] = static_cast<uint8_t>(std::clamp(comp.bgR, 0, 255));
        out.rgba[i + 1] = static_cast<uint8_t>(std::clamp(comp.bgG, 0, 255));
        out.rgba[i + 2] = static_cast<uint8_t>(std::clamp(comp.bgB, 0, 255));
        out.rgba[i + 3] = 255;
    }
    const double osx = static_cast<double>(ow) / comp.width;
    const double osy = static_cast<double>(oh) / comp.height;

    bool anySolo = false;
    for (const Layer& l : comp.layers) if (l.solo && l.visible) anySolo = true;

    for (int i = static_cast<int>(comp.layers.size()) - 1; i >= 0; --i) {   // bottom to top
        const Layer& L = comp.layers[static_cast<size_t>(i)];
        if (!L.visible || (anySolo && !L.solo)) continue;
        if (time < L.inPoint || time >= L.outPoint) continue;

        const double lt = time - L.startTime;   // layer-local time (keyframes & source time)
        const double opacity = std::clamp(L.prop(Prop::Opacity).valueAt(lt) / 100.0, 0.0, 1.0);
        if (opacity <= 0) continue;

        Xform X;
        X.ax = L.prop(Prop::AnchorX).valueAt(lt); X.ay = L.prop(Prop::AnchorY).valueAt(lt);
        X.px = L.prop(Prop::PosX).valueAt(lt);    X.py = L.prop(Prop::PosY).valueAt(lt);
        X.sx = L.prop(Prop::ScaleX).valueAt(lt) / 100.0; X.sy = L.prop(Prop::ScaleY).valueAt(lt) / 100.0;
        const double rad = L.prop(Prop::Rotation).valueAt(lt) * 3.14159265358979323846 / 180.0;
        X.c = std::cos(rad); X.s = std::sin(rad);

        if (L.type == LayerType::Solid) {
            const uint8_t col[3] = {static_cast<uint8_t>(std::clamp(L.solidR, 0, 255)),
                                    static_cast<uint8_t>(std::clamp(L.solidG, 0, 255)),
                                    static_cast<uint8_t>(std::clamp(L.solidB, 0, 255))};
            drawLayer(out, X, opacity, osx, osy, std::max(1, L.solidW), std::max(1, L.solidH), nullptr, col);
        } else {
            const Footage* f = project.findFootage(L.footageId);
            if (!f || f->kind == FootageKind::Audio) continue;
            const Frame* fr = source.frame(L.id, f->path, std::max(0.0, lt));
            if (!fr || fr->empty()) continue;
            const uint8_t none[3] = {0, 0, 0};
            drawLayer(out, X, opacity, osx, osy, fr->width, fr->height, fr, none);
        }
    }
}
}
