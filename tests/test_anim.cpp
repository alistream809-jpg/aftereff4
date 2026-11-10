#include "TestHarness.h"
#include "anim/Property.h"
using namespace at;

TEST(anim_static_value) {
    AnimatedProperty p(42);
    CHECK_NEAR(p.valueAt(-5), 42, 0); CHECK_NEAR(p.valueAt(100), 42, 0);
    CHECK(!p.isAnimated());
}
TEST(anim_linear) {
    AnimatedProperty p(0);
    p.setKey(1.0, 10); p.setKey(3.0, 50);
    CHECK_NEAR(p.valueAt(0), 10, 1e-12);     // before first key
    CHECK_NEAR(p.valueAt(2.0), 30, 1e-12);
    CHECK_NEAR(p.valueAt(9), 50, 1e-12);     // after last key
}
TEST(anim_keys_sorted_and_replaced) {
    AnimatedProperty p(0);
    p.setKey(2, 20); p.setKey(0, 0); p.setKey(1, 10); p.setKey(1, 15);
    CHECK(p.keys().size() == 3);
    CHECK(p.keys()[0].time == 0 && p.keys()[1].time == 1 && p.keys()[2].time == 2);
    CHECK_NEAR(p.keys()[1].value, 15, 0);
    CHECK(p.removeKeyAt(1));
    CHECK(p.keys().size() == 2);
}
TEST(anim_hold) {
    AnimatedProperty p(0);
    p.setKey(0, 5, Interp::Hold); p.setKey(1, 9);
    CHECK_NEAR(p.valueAt(0.99), 5, 0);
    CHECK_NEAR(p.valueAt(1.0), 9, 0);
}
TEST(anim_bezier_easy_ease) {
    AnimatedProperty p(0);
    p.setKey(0, 0, Interp::Bezier); p.setKey(1, 100);
    CHECK_NEAR(p.valueAt(0.5), 50, 0.01);          // symmetric curve passes through the middle
    CHECK(p.valueAt(0.1) < 10.0);                  // slow start (ease in)
    CHECK(p.valueAt(0.9) > 90.0);                  // slow end (ease out)
    double prev = -1;
    for (int i = 0; i <= 100; ++i) { double v = p.valueAt(i / 100.0); CHECK(v >= prev - 1e-9); prev = v; }
}
TEST(anim_bezier_linear_handles_equal_linear) {
    for (int i = 0; i <= 10; ++i) CHECK_NEAR(cubicBezierEase(0.33, 0.33, 0.66, 0.66, i / 10.0), i / 10.0, 0.01);
}
TEST(anim_json_roundtrip) {
    AnimatedProperty p(7);
    p.setKey(0, 1, Interp::Bezier); p.setKey(2.5, 9, Interp::Hold); p.setKey(4, -3);
    p.keys()[0].x1 = 0.2; p.keys()[0].y2 = 0.9;
    AnimatedProperty q = AnimatedProperty::fromJson(Json::parse(p.toJson().dump()), 0);
    CHECK(q.keys().size() == 3);
    CHECK(q.keys()[0].interp == Interp::Bezier && q.keys()[1].interp == Interp::Hold && q.keys()[2].interp == Interp::Linear);
    CHECK_NEAR(q.keys()[0].x1, 0.2, 0); CHECK_NEAR(q.keys()[0].y2, 0.9, 0);
    for (double t : {0.0, 0.7, 2.4, 3.0, 5.0}) CHECK_NEAR(p.valueAt(t), q.valueAt(t), 1e-12);
}
