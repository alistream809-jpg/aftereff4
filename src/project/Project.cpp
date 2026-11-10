#include "project/Project.h"
#include <algorithm>
#include <cmath>
#include "core/FileUtil.h"
#include "core/Id.h"

namespace at {

const char* propLabel(Prop p) {
    static const char* n[] = {"Anchor Point X", "Anchor Point Y", "Position X", "Position Y",
                              "Scale X (%)", "Scale Y (%)", "Rotation (deg)", "Opacity (%)"};
    return n[static_cast<int>(p)];
}
const char* propJsonKey(Prop p) {
    static const char* n[] = {"anchorX", "anchorY", "posX", "posY", "scaleX", "scaleY", "rotation", "opacity"};
    return n[static_cast<int>(p)];
}
double propDefault(Prop p) { return (p == Prop::ScaleX || p == Prop::ScaleY || p == Prop::Opacity) ? 100.0 : 0.0; }

Layer::Layer() {
    for (int i = 0; i < kPropCount; ++i) props[i].setStaticValue(propDefault(static_cast<Prop>(i)));
}

Json Layer::toJson() const {
    Json j = Json::object();
    j["id"] = id; j["name"] = name;
    j["type"] = type == LayerType::Solid ? "solid" : "footage";
    if (type == LayerType::Footage) j["footage"] = footageId;
    else {
        Json s = Json::object();
        Json c = Json::array(); c.push(solidR); c.push(solidG); c.push(solidB);
        s["color"] = c; s["width"] = solidW; s["height"] = solidH;
        j["solid"] = s;
    }
    j["start"] = startTime; j["in"] = inPoint; j["out"] = outPoint;
    j["visible"] = visible; j["locked"] = locked; j["solo"] = solo;
    Json pr = Json::object();
    for (int i = 0; i < kPropCount; ++i) pr[propJsonKey(static_cast<Prop>(i))] = props[i].toJson();
    j["props"] = pr;
    return j;
}

Layer Layer::fromJson(const Json& j) {
    if (!j.isObject()) throw std::runtime_error("Layer entry is not an object");
    Layer l;
    l.id = j.getString("id", newId());
    l.name = j.getString("name", "Layer");
    l.type = j.getString("type", "footage") == "solid" ? LayerType::Solid : LayerType::Footage;
    l.footageId = j.getString("footage", "");
    const Json& s = j.get("solid");
    if (s.isObject()) {
        const Json& c = s.get("color");
        if (c.isArray() && c.asArray().size() == 3 && c.asArray()[0].isNumber() && c.asArray()[1].isNumber() && c.asArray()[2].isNumber()) {
            l.solidR = static_cast<int>(c.asArray()[0].asNumber());
            l.solidG = static_cast<int>(c.asArray()[1].asNumber());
            l.solidB = static_cast<int>(c.asArray()[2].asNumber());
        }
        l.solidW = static_cast<int>(s.getNumber("width", 0));
        l.solidH = static_cast<int>(s.getNumber("height", 0));
    }
    l.startTime = j.getNumber("start", 0);
    l.inPoint = j.getNumber("in", 0);
    l.outPoint = j.getNumber("out", 5);
    l.visible = j.getBool("visible", true);
    l.locked = j.getBool("locked", false);
    l.solo = j.getBool("solo", false);
    const Json& pr = j.get("props");
    for (int i = 0; i < kPropCount; ++i) {
        Prop p = static_cast<Prop>(i);
        l.props[i] = AnimatedProperty::fromJson(pr.get(propJsonKey(p)), propDefault(p));
    }
    return l;
}

Layer* Composition::findLayer(const std::string& lid) {
    for (auto& l : layers) if (l.id == lid) return &l;
    return nullptr;
}
const Layer* Composition::findLayer(const std::string& lid) const {
    for (auto& l : layers) if (l.id == lid) return &l;
    return nullptr;
}
int Composition::layerIndex(const std::string& lid) const {
    for (size_t i = 0; i < layers.size(); ++i) if (layers[i].id == lid) return static_cast<int>(i);
    return -1;
}

Json Composition::toJson() const {
    Json j = Json::object();
    j["id"] = id; j["name"] = name; j["width"] = width; j["height"] = height;
    Json f = Json::object(); f["num"] = fps.num; f["den"] = fps.den;
    j["fps"] = f;
    j["duration"] = duration;
    Json bg = Json::array(); bg.push(bgR); bg.push(bgG); bg.push(bgB);
    j["background"] = bg;
    Json ls = Json::array();
    for (const Layer& l : layers) ls.push(l.toJson());
    j["layers"] = ls;
    return j;
}

Composition Composition::fromJson(const Json& j) {
    if (!j.isObject()) throw std::runtime_error("Composition entry is not an object");
    Composition c;
    c.id = j.getString("id", newId());
    c.name = j.getString("name", "Comp");
    c.width = std::clamp(static_cast<int>(j.getNumber("width", 1920)), 1, 16384);
    c.height = std::clamp(static_cast<int>(j.getNumber("height", 1080)), 1, 16384);
    const Json& f = j.get("fps");
    c.fps.num = std::max(1, static_cast<int>(f.getNumber("num", 30)));
    c.fps.den = std::max(1, static_cast<int>(f.getNumber("den", 1)));
    c.duration = std::max(0.04, j.getNumber("duration", 10));
    const Json& bg = j.get("background");
    if (bg.isArray() && bg.asArray().size() == 3 && bg.asArray()[0].isNumber() && bg.asArray()[1].isNumber() && bg.asArray()[2].isNumber()) {
        c.bgR = static_cast<int>(bg.asArray()[0].asNumber());
        c.bgG = static_cast<int>(bg.asArray()[1].asNumber());
        c.bgB = static_cast<int>(bg.asArray()[2].asNumber());
    }
    const Json& ls = j.get("layers");
    if (ls.isArray()) for (const Json& lj : ls.asArray()) c.layers.push_back(Layer::fromJson(lj));
    return c;
}

Footage* Project::findFootage(const std::string& i) { for (auto& f : footage) if (f.id == i) return &f; return nullptr; }
const Footage* Project::findFootage(const std::string& i) const { for (auto& f : footage) if (f.id == i) return &f; return nullptr; }
Composition* Project::findComp(const std::string& i) { for (auto& c : comps) if (c.id == i) return &c; return nullptr; }
const Composition* Project::findComp(const std::string& i) const { for (auto& c : comps) if (c.id == i) return &c; return nullptr; }
bool Project::footageInUse(const std::string& fid) const {
    for (const auto& c : comps) for (const auto& l : c.layers)
        if (l.type == LayerType::Footage && l.footageId == fid) return true;
    return false;
}

static const char* kindName(FootageKind k) {
    switch (k) { case FootageKind::Image: return "image"; case FootageKind::Audio: return "audio"; default: return "video"; }
}

Json Project::toJson() const {
    Json j = Json::object();
    j["format"] = "AFTERTRUE";
    j["version"] = kProjectFormatVersion;
    j["name"] = name;
    j["activeComp"] = activeCompId;
    Json fs = Json::array();
    for (const Footage& f : footage) {
        Json fj = Json::object();
        fj["id"] = f.id; fj["name"] = f.name; fj["path"] = f.path; fj["kind"] = kindName(f.kind);
        fj["width"] = f.width; fj["height"] = f.height; fj["fps"] = f.fps;
        fj["duration"] = f.duration; fj["hasAudio"] = f.hasAudio;
        fs.push(fj);
    }
    j["footage"] = fs;
    Json cs = Json::array();
    for (const Composition& c : comps) cs.push(c.toJson());
    j["compositions"] = cs;
    return j;
}

Project Project::fromJson(const Json& j) {
    if (!j.isObject() || j.getString("format", "") != "AFTERTRUE")
        throw std::runtime_error("This is not an AFTER TRUE project file.");
    const int ver = static_cast<int>(j.getNumber("version", 0));
    if (ver > kProjectFormatVersion)
        throw std::runtime_error("This project was saved by a newer version of AFTER TRUE (format " + std::to_string(ver) + "). Please update the application.");
    if (ver < 1) throw std::runtime_error("Unknown project format version.");
    Project p;
    p.name = j.getString("name", "Untitled");
    p.activeCompId = j.getString("activeComp", "");
    const Json& fs = j.get("footage");
    if (fs.isArray()) for (const Json& fj : fs.asArray()) {
        Footage f;
        f.id = fj.getString("id", newId());
        f.name = fj.getString("name", "Footage");
        f.path = fj.getString("path", "");
        const std::string k = fj.getString("kind", "video");
        f.kind = k == "image" ? FootageKind::Image : (k == "audio" ? FootageKind::Audio : FootageKind::Video);
        f.width = static_cast<int>(fj.getNumber("width", 0));
        f.height = static_cast<int>(fj.getNumber("height", 0));
        f.fps = fj.getNumber("fps", 0);
        f.duration = fj.getNumber("duration", 0);
        f.hasAudio = fj.getBool("hasAudio", false);
        p.footage.push_back(f);
    }
    const Json& cs = j.get("compositions");
    if (cs.isArray()) for (const Json& cj : cs.asArray()) p.comps.push_back(Composition::fromJson(cj));
    if (!p.findComp(p.activeCompId)) p.activeCompId = p.comps.empty() ? "" : p.comps.front().id;
    return p;
}

Composition makeComposition(const std::string& name, int w, int h, Rational fps, double duration) {
    Composition c;
    c.id = newId(); c.name = name; c.width = w; c.height = h; c.fps = fps; c.duration = duration;
    return c;
}

Project makeDefaultProject() {
    Project p;
    Composition c = makeComposition("Comp 1", 1920, 1080, Rational{30, 1}, 10.0);
    p.activeCompId = c.id;
    p.comps.push_back(std::move(c));
    return p;
}

Layer makeFootageLayer(const Footage& f, const Composition& c, double atTime) {
    Layer l;
    l.id = newId(); l.name = f.name; l.type = LayerType::Footage; l.footageId = f.id;
    l.startTime = atTime; l.inPoint = atTime;
    if (f.kind == FootageKind::Image) l.outPoint = atTime < c.duration ? c.duration : atTime + 5.0;
    else l.outPoint = atTime + std::max(f.duration, 0.04);
    if (f.width > 0 && f.height > 0) {
        const double s = std::min(static_cast<double>(c.width) / f.width, static_cast<double>(c.height) / f.height) * 100.0;
        l.prop(Prop::ScaleX).setStaticValue(s);
        l.prop(Prop::ScaleY).setStaticValue(s);
        l.prop(Prop::AnchorX).setStaticValue(f.width / 2.0);
        l.prop(Prop::AnchorY).setStaticValue(f.height / 2.0);
    }
    l.prop(Prop::PosX).setStaticValue(c.width / 2.0);
    l.prop(Prop::PosY).setStaticValue(c.height / 2.0);
    return l;
}

Layer makeSolidLayer(const std::string& name, int r, int g, int b, const Composition& c, double atTime) {
    Layer l;
    l.id = newId(); l.name = name; l.type = LayerType::Solid;
    l.solidR = r; l.solidG = g; l.solidB = b; l.solidW = c.width; l.solidH = c.height;
    l.startTime = atTime; l.inPoint = atTime; l.outPoint = atTime < c.duration ? c.duration : atTime + 5.0;
    l.prop(Prop::AnchorX).setStaticValue(c.width / 2.0);
    l.prop(Prop::AnchorY).setStaticValue(c.height / 2.0);
    l.prop(Prop::PosX).setStaticValue(c.width / 2.0);
    l.prop(Prop::PosY).setStaticValue(c.height / 2.0);
    return l;
}

bool saveProjectFile(const Project& p, const std::string& path, std::string& err) {
    return writeTextFileAtomic(path, p.toJson().dump(2), err);
}

bool loadProjectFile(const std::string& path, Project& out, std::string& err) {
    std::string text;
    if (!readTextFile(path, text, err)) return false;
    try {
        out = Project::fromJson(Json::parse(text));
    } catch (const std::exception& e) {
        err = e.what();
        return false;
    }
    return true;
}
}
