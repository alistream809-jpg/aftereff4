#include "TestHarness.h"
#include <filesystem>
#include "project/Project.h"
using namespace at;

static Project sampleProject() {
    Project p = makeDefaultProject();
    Footage f; f.id = "foot1"; f.name = "clip \xD0\x92.mp4"; f.path = "C:\\Videos\\clip.mp4";
    f.kind = FootageKind::Video; f.width = 3840; f.height = 2160; f.fps = 29.97; f.duration = 12.5; f.hasAudio = true;
    p.footage.push_back(f);
    Composition& c = p.comps[0];
    c.layers.push_back(makeFootageLayer(f, c, 1.0));
    Layer s = makeSolidLayer("Red", 255, 0, 0, c, 0);
    s.prop(Prop::Opacity).setKey(0, 0, Interp::Bezier);
    s.prop(Prop::Opacity).setKey(2, 100);
    s.locked = true; s.solo = true;
    c.layers.insert(c.layers.begin(), s);
    return p;
}

TEST(project_roundtrip_in_memory) {
    Project p = sampleProject();
    Project q = Project::fromJson(Json::parse(p.toJson().dump()));
    CHECK(q.footage.size() == 1 && q.comps.size() == 1);
    CHECK(q.footage[0].name == p.footage[0].name);
    CHECK(q.footage[0].width == 3840 && q.footage[0].hasAudio);
    const Composition& c = q.comps[0];
    CHECK(c.layers.size() == 2);
    CHECK(c.layers[0].type == LayerType::Solid && c.layers[0].solidR == 255 && c.layers[0].locked && c.layers[0].solo);
    CHECK(c.layers[0].prop(Prop::Opacity).keys().size() == 2);
    CHECK_NEAR(c.layers[0].prop(Prop::Opacity).valueAt(1.0), p.comps[0].layers[0].prop(Prop::Opacity).valueAt(1.0), 1e-12);
    CHECK(c.layers[1].footageId == "foot1");
    CHECK_NEAR(c.layers[1].startTime, 1.0, 0);
    CHECK_NEAR(c.layers[1].prop(Prop::ScaleX).staticValue(), 50.0, 1e-9);   // 3840x2160 fit into 1920x1080
    CHECK(q.activeCompId == p.activeCompId);
}
TEST(project_save_and_reopen_file) {
    Project p = sampleProject();
    std::string path = (std::filesystem::temp_directory_path() / "at_test_project.atrue").string();
    std::string err;
    CHECK(saveProjectFile(p, path, err));
    Project q;
    CHECK(loadProjectFile(path, q, err));
    CHECK(q.toJson().dump() == p.toJson().dump());   // lossless
    std::filesystem::remove(path);
}
TEST(project_rejects_bad_files) {
    Project q; std::string err;
    CHECK(!loadProjectFile("/definitely/not/here.atrue", q, err));
    bool threw = false;
    try { Project::fromJson(Json::parse("{\"format\":\"OTHER\"}")); } catch (const std::runtime_error&) { threw = true; }
    CHECK(threw);
    threw = false; std::string msg;
    try { Project::fromJson(Json::parse("{\"format\":\"AFTERTRUE\",\"version\":999}")); } catch (const std::runtime_error& e) { threw = true; msg = e.what(); }
    CHECK(threw && msg.find("newer") != std::string::npos);
}
TEST(project_footage_in_use) {
    Project p = sampleProject();
    CHECK(p.footageInUse("foot1"));
    CHECK(!p.footageInUse("nope"));
}
