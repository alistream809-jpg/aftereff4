#include "anim/Property.h"
#include <algorithm>
#include <cmath>
namespace at {

double cubicBezierEase(double x1, double y1, double x2, double y2, double x) {
    if (x <= 0) return 0;
    if (x >= 1) return 1;
    x1 = std::clamp(x1, 0.0, 1.0);
    x2 = std::clamp(x2, 0.0, 1.0);
    auto bez = [](double a, double b, double s) {   // cubic with endpoints 0 and 1
        const double inv = 1 - s;
        return 3 * inv * inv * s * a + 3 * inv * s * s * b + s * s * s;
    };
    double lo = 0, hi = 1, s = x;
    for (int i = 0; i < 40; ++i) {                  // bisection: robust for any handle values
        s = 0.5 * (lo + hi);
        const double bx = bez(x1, x2, s);
        if (std::fabs(bx - x) < 1e-9) break;
        if (bx < x) lo = s; else hi = s;
    }
    return bez(y1, y2, s);
}

double AnimatedProperty::valueAt(double t) const {
    if (keys_.empty()) return value_;
    if (t <= keys_.front().time) return keys_.front().value;
    if (t >= keys_.back().time) return keys_.back().value;
    auto it = std::upper_bound(keys_.begin(), keys_.end(), t,
                               [](double tt, const Keyframe& k) { return tt < k.time; });
    const Keyframe& b = *it;
    const Keyframe& a = *(it - 1);
    const double span = b.time - a.time;
    if (span <= 0) return b.value;
    const double u = (t - a.time) / span;
    switch (a.interp) {
        case Interp::Hold: return a.value;
        case Interp::Bezier: return a.value + (b.value - a.value) * cubicBezierEase(a.x1, a.y1, a.x2, a.y2, u);
        case Interp::Linear: default: return a.value + (b.value - a.value) * u;
    }
}

int AnimatedProperty::keyIndexAt(double t, double eps) const {
    for (size_t i = 0; i < keys_.size(); ++i)
        if (std::fabs(keys_[i].time - t) <= eps) return static_cast<int>(i);
    return -1;
}

void AnimatedProperty::setKey(double t, double v, Interp interp) {
    int idx = keyIndexAt(t);
    if (idx >= 0) { keys_[idx].value = v; return; }   // keep the existing key's interpolation
    Keyframe k; k.time = t; k.value = v; k.interp = interp;
    auto it = std::upper_bound(keys_.begin(), keys_.end(), t,
                               [](double tt, const Keyframe& kk) { return tt < kk.time; });
    keys_.insert(it, k);
}

bool AnimatedProperty::removeKeyAt(double t, double eps) {
    int idx = keyIndexAt(t, eps);
    if (idx < 0) return false;
    keys_.erase(keys_.begin() + idx);
    return true;
}

static const char* interpName(Interp i) {
    switch (i) { case Interp::Hold: return "hold"; case Interp::Bezier: return "bezier"; default: return "linear"; }
}

Json AnimatedProperty::toJson() const {
    Json j = Json::object();
    j["value"] = value_;
    if (!keys_.empty()) {
        Json arr = Json::array();
        for (const Keyframe& k : keys_) {
            Json kj = Json::object();
            kj["t"] = k.time;
            kj["v"] = k.value;
            kj["i"] = interpName(k.interp);
            if (k.interp == Interp::Bezier) {
                Json b = Json::array();
                b.push(k.x1); b.push(k.y1); b.push(k.x2); b.push(k.y2);
                kj["b"] = b;
            }
            arr.push(kj);
        }
        j["keys"] = arr;
    }
    return j;
}

AnimatedProperty AnimatedProperty::fromJson(const Json& j, double def) {
    AnimatedProperty p(j.getNumber("value", def));
    const Json& keys = j.get("keys");
    if (keys.isArray()) {
        for (const Json& kj : keys.asArray()) {
            Keyframe k;
            k.time = kj.getNumber("t", 0);
            k.value = kj.getNumber("v", 0);
            const std::string i = kj.getString("i", "linear");
            k.interp = i == "hold" ? Interp::Hold : (i == "bezier" ? Interp::Bezier : Interp::Linear);
            const Json& b = kj.get("b");
            if (b.isArray() && b.asArray().size() == 4) {
                const auto& a = b.asArray();
                if (a[0].isNumber() && a[1].isNumber() && a[2].isNumber() && a[3].isNumber()) {
                    k.x1 = a[0].asNumber(); k.y1 = a[1].asNumber();
                    k.x2 = a[2].asNumber(); k.y2 = a[3].asNumber();
                }
            }
            p.keys_.push_back(k);
        }
        std::sort(p.keys_.begin(), p.keys_.end(), [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
    }
    return p;
}
}
