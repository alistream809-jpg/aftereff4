#pragma once
#include <string>
#include <vector>
#include "anim/Property.h"
#include "core/Json.h"
#include "core/Rational.h"

namespace at {

constexpr int kProjectFormatVersion = 1;

enum class Prop { AnchorX, AnchorY, PosX, PosY, ScaleX, ScaleY, Rotation, Opacity, Count };
constexpr int kPropCount = static_cast<int>(Prop::Count);
const char* propLabel(Prop p);     // "Position X"
const char* propJsonKey(Prop p);   // "posX"
double propDefault(Prop p);        // scale/opacity = 100, others 0

enum class FootageKind { Video, Image, Audio };
enum class LayerType { Footage, Solid };

struct Footage {
    std::string id, name, path;   // path is UTF-8, absolute
    FootageKind kind = FootageKind::Video;
    int width = 0, height = 0;
    double fps = 0;
    double duration = 0;          // seconds (0 for stills)
    bool hasAudio = false;
};

struct Layer {
    std::string id, name;
    LayerType type = LayerType::Footage;
    std::string footageId;                 // for footage layers
    int solidR = 128, solidG = 128, solidB = 128, solidW = 0, solidH = 0;   // for solids
    double startTime = 0;                  // composition time at which source/keyframe time 0 sits
    double inPoint = 0, outPoint = 5;      // visible range in composition time
    bool visible = true, locked = false, solo = false;
    AnimatedProperty props[kPropCount];

    Layer();
    AnimatedProperty& prop(Prop p) { return props[static_cast<int>(p)]; }
    const AnimatedProperty& prop(Prop p) const { return props[static_cast<int>(p)]; }
    Json toJson() const;
    static Layer fromJson(const Json& j);
};

struct Composition {
    std::string id, name = "Comp";
    int width = 1920, height = 1080;
    Rational fps{30, 1};
    double duration = 10;
    int bgR = 0, bgG = 0, bgB = 0;
    std::vector<Layer> layers;             // index 0 = TOP layer (like After Effects)

    Layer* findLayer(const std::string& layerId);
    const Layer* findLayer(const std::string& layerId) const;
    int layerIndex(const std::string& layerId) const;
    Json toJson() const;
    static Composition fromJson(const Json& j);
};

struct Project {
    std::string name = "Untitled";
    std::vector<Footage> footage;
    std::vector<Composition> comps;
    std::string activeCompId;

    Footage* findFootage(const std::string& id);
    const Footage* findFootage(const std::string& id) const;
    Composition* findComp(const std::string& id);
    const Composition* findComp(const std::string& id) const;
    bool footageInUse(const std::string& footageId) const;

    Json toJson() const;
    static Project fromJson(const Json& j);   // throws std::runtime_error on invalid/newer files
};

// Factories
Composition makeComposition(const std::string& name, int w, int h, Rational fps, double duration);
Project makeDefaultProject();
// Creates a footage layer at composition time 'atTime', scaled to fit and centred.
Layer makeFootageLayer(const Footage& f, const Composition& c, double atTime);
Layer makeSolidLayer(const std::string& name, int r, int g, int b, const Composition& c, double atTime);

// Saving / loading ".atrue" project files
bool saveProjectFile(const Project& p, const std::string& utf8Path, std::string& err);
bool loadProjectFile(const std::string& utf8Path, Project& out, std::string& err);

}
