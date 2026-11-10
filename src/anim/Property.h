#pragma once
#include <vector>
#include "core/Json.h"
namespace at {

enum class Interp { Linear, Hold, Bezier };

// One keyframe. 'interp' describes the segment that STARTS at this key.
// For Bezier the segment uses a CSS-style cubic-bezier (x1,y1,x2,y2) in
// normalised segment space; the defaults (0.42,0,0.58,1) are "Easy Ease".
struct Keyframe {
    double time = 0;     // layer-local seconds
    double value = 0;
    Interp interp = Interp::Linear;
    double x1 = 0.42, y1 = 0.0, x2 = 0.58, y2 = 1.0;
};

// Solves a cubic-bezier easing curve: returns y for a given x in [0,1].
double cubicBezierEase(double x1, double y1, double x2, double y2, double x);

class AnimatedProperty {
public:
    AnimatedProperty() = default;
    explicit AnimatedProperty(double v) : value_(v) {}

    bool isAnimated() const { return !keys_.empty(); }
    double valueAt(double t) const;                 // evaluates keyframes (or the static value)
    double staticValue() const { return value_; }
    void setStaticValue(double v) { value_ = v; }

    // Inserts a key, or replaces the one at (nearly) the same time.
    void setKey(double t, double v, Interp interp = Interp::Linear);
    bool removeKeyAt(double t, double eps = 1e-6);
    void clearKeys() { keys_.clear(); }
    int keyIndexAt(double t, double eps = 1e-6) const;
    const std::vector<Keyframe>& keys() const { return keys_; }
    std::vector<Keyframe>& keys() { return keys_; }

    Json toJson() const;
    static AnimatedProperty fromJson(const Json& j, double defaultValue);

private:
    double value_ = 0;
    std::vector<Keyframe> keys_;   // always sorted by time
};
}
