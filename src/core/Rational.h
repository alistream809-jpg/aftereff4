#pragma once
namespace at {
// Exact frame rate such as 30000/1001 (29.97 fps).
struct Rational {
    int num = 30;
    int den = 1;
    double toDouble() const { return den != 0 ? static_cast<double>(num) / den : 0.0; }
    bool operator==(const Rational& o) const { return num == o.num && den == o.den; }
};
}
