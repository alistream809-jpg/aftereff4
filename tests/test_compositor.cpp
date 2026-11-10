#include "TestHarness.h"
#include <map>
#include "comp/Compositor.h"
using namespace at;

struct FakeSource : IFrameSource {
    std::map<std::string, Frame> frames;   // by path
    const Frame* frame(const std::string&, const std::string& path, double) override {
        auto it = frames.find(path); return it == frames.end() ? nullptr : &it->second;
    }
};
static const uint8_t* px(const Frame& f, int x, int y) { return &f.rgba[(static_cast<size_t>(y) * f.width + x) * 4]; }

static Composition comp100() { return makeComposition("t", 100, 100, Rational{30, 1}, 5); }

TEST(comp_solid_fills_and_background) {
    Project p; Composition c = comp100(); c.bgR = 10; c.bgG = 20; c.bgB = 30;
    c.layers.push_back(makeSolidLayer("s", 255, 0, 0, c, 0));
    c.layers[0].prop(Prop::ScaleX).setStaticValue(50); c.layers[0].prop(Prop::ScaleY).setStaticValue(50);
    FakeSource src; Frame out;
    renderComposition(p, c, 0, src, out);
    CHECK(out.width == 100 && out.height == 100);
    CHECK(px(out, 50, 50)[0] == 255 && px(out, 50, 50)[1] == 0);   // inside 50x50 centred square
    CHECK(px(out, 5, 5)[0] == 10 && px(out, 5, 5)[1] == 20 && px(out, 5, 5)[2] == 30);   // background
    CHECK(px(out, 30, 30)[0] == 255);   CHECK(px(out, 22, 50)[0] == 10);
}
TEST(comp_opacity_blend_and_stack_order) {
    Project p; Composition c = comp100();
    c.layers.push_back(makeSolidLayer("blue", 0, 0, 255, c, 0));   // index 0 = top
    c.layers.push_back(makeSolidLayer("red", 255, 0, 0, c, 0));    // bottom
    c.layers[0].prop(Prop::Opacity).setStaticValue(50);
    FakeSource src; Frame out;
    renderComposition(p, c, 0, src, out);
    CHECK_NEAR(px(out, 50, 50)[0], 127.5, 1.0); CHECK_NEAR(px(out, 50, 50)[2], 127.5, 1.0);
    c.layers[0].prop(Prop::Opacity).setStaticValue(100);
    renderComposition(p, c, 0, src, out);
    CHECK(px(out, 50, 50)[2] == 255 && px(out, 50, 50)[0] == 0);   // top layer wins
}
TEST(comp_keyframed_opacity_changes_pixels) {
    Project p; Composition c = comp100();
    Layer s = makeSolidLayer("w", 255, 255, 255, c, 0);
    s.prop(Prop::Opacity).setKey(0, 0); s.prop(Prop::Opacity).setKey(1, 100);
    c.layers.push_back(s);
    FakeSource src; Frame out;
    renderComposition(p, c, 0.0, src, out); CHECK(px(out, 1, 1)[0] == 0);
    renderComposition(p, c, 0.5, src, out); CHECK_NEAR(px(out, 1, 1)[0], 127.5, 1.0);
    renderComposition(p, c, 1.0, src, out); CHECK(px(out, 1, 1)[0] == 255);
}
TEST(comp_keyframes_follow_layer_start) {
    Project p; Composition c = comp100();
    Layer s = makeSolidLayer("w", 255, 255, 255, c, 0);
    s.prop(Prop::Opacity).setKey(0, 0); s.prop(Prop::Opacity).setKey(1, 100);
    s.startTime = 2.0; s.inPoint = 2.0; s.outPoint = 4.0;       // moved later in the comp
    c.layers.push_back(s);
    FakeSource src; Frame out;
    renderComposition(p, c, 1.0, src, out); CHECK(px(out, 1, 1)[0] == 0);      // not yet visible
    renderComposition(p, c, 2.5, src, out); CHECK_NEAR(px(out, 1, 1)[0], 127.5, 1.0);
    renderComposition(p, c, 4.0, src, out); CHECK(px(out, 1, 1)[0] == 0);      // past out point
}
TEST(comp_visibility_and_solo) {
    Project p; Composition c = comp100();
    c.layers.push_back(makeSolidLayer("a", 255, 0, 0, c, 0));
    c.layers.push_back(makeSolidLayer("b", 0, 255, 0, c, 0));
    FakeSource src; Frame out;
    c.layers[0].visible = false;
    renderComposition(p, c, 0, src, out); CHECK(px(out, 50, 50)[1] == 255 && px(out, 50, 50)[0] == 0);
    c.layers[0].visible = true; c.layers[1].solo = true;
    renderComposition(p, c, 0, src, out); CHECK(px(out, 50, 50)[1] == 255 && px(out, 50, 50)[0] == 0);   // only solo shown
}
TEST(comp_position_and_rotation) {
    Project p; Composition c = comp100();
    Layer s = makeSolidLayer("bar", 255, 255, 255, c, 0);
    s.solidW = 40; s.solidH = 20;
    s.prop(Prop::AnchorX).setStaticValue(20); s.prop(Prop::AnchorY).setStaticValue(10);
    s.prop(Prop::PosX).setStaticValue(50); s.prop(Prop::PosY).setStaticValue(50);
    c.layers.push_back(s);
    FakeSource src; Frame out;
    renderComposition(p, c, 0, src, out);
    CHECK(px(out, 65, 50)[0] == 255);  CHECK(px(out, 50, 65)[0] == 0);   // wide, not tall
    c.layers[0].prop(Prop::Rotation).setStaticValue(90);
    renderComposition(p, c, 0, src, out);
    CHECK(px(out, 50, 65)[0] == 255);  CHECK(px(out, 65, 50)[0] == 0);   // rotated to tall
    c.layers[0].prop(Prop::Rotation).setStaticValue(0);
    c.layers[0].prop(Prop::PosX).setStaticValue(20);
    renderComposition(p, c, 0, src, out);
    CHECK(px(out, 20, 50)[0] == 255);  CHECK(px(out, 70, 50)[0] == 0);
}
TEST(comp_footage_pixel_exact_mapping) {
    Project p;
    Footage f; f.id = "f"; f.name = "f"; f.path = "p"; f.width = 2; f.height = 1; f.kind = FootageKind::Image;
    p.footage.push_back(f);
    Composition c = makeComposition("t", 2, 1, Rational{30, 1}, 1);
    Layer l = makeFootageLayer(f, c, 0);   // scale 100, centred
    c.layers.push_back(l);
    FakeSource src; Frame img; img.allocate(2, 1);
    uint8_t* d = img.rgba.data();
    d[0] = 255; d[1] = 0; d[2] = 0; d[3] = 255;       // red
    d[4] = 0; d[5] = 0; d[6] = 255; d[7] = 255;       // blue
    src.frames["p"] = img;
    Frame out;
    renderComposition(p, c, 0, src, out);
    CHECK(out.width == 2 && px(out, 0, 0)[0] == 255 && px(out, 0, 0)[2] == 0);
    CHECK(px(out, 1, 0)[2] == 255 && px(out, 1, 0)[0] == 0);
}
TEST(comp_transparent_source_pixels_do_not_draw) {
    Project p;
    Footage f; f.id = "f"; f.path = "p"; f.width = 2; f.height = 1; f.kind = FootageKind::Image;
    p.footage.push_back(f);
    Composition c = makeComposition("t", 2, 1, Rational{30, 1}, 1);
    c.layers.push_back(makeFootageLayer(f, c, 0));
    FakeSource src; Frame img; img.allocate(2, 1);
    img.rgba[0] = 255; img.rgba[3] = 255;   // opaque red; second pixel fully transparent
    src.frames["p"] = img;
    Frame out; renderComposition(p, c, 0, src, out);
    CHECK(px(out, 0, 0)[0] == 255);
    CHECK(px(out, 1, 0)[0] == 0 && px(out, 1, 0)[1] == 0);
}
TEST(comp_preview_divisor) {
    Project p; Composition c = comp100();
    c.layers.push_back(makeSolidLayer("s", 0, 200, 0, c, 0));
    FakeSource src; Frame out;
    renderComposition(p, c, 0, src, out, 4);
    CHECK(out.width == 25 && out.height == 25);
    CHECK(px(out, 12, 12)[1] == 200 && px(out, 0, 0)[1] == 200 && px(out, 24, 24)[1] == 200);
}
TEST(comp_missing_media_is_skipped) {
    Project p;
    Footage f; f.id = "f"; f.path = "missing"; f.width = 10; f.height = 10; f.kind = FootageKind::Video;
    p.footage.push_back(f);
    Composition c = comp100();
    c.layers.push_back(makeFootageLayer(f, c, 0));
    FakeSource src; Frame out;
    renderComposition(p, c, 0, src, out);
    CHECK(px(out, 50, 50)[0] == 0);   // nothing drawn, no crash
}
