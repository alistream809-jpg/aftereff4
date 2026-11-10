#include "ui/Document.h"
#include <algorithm>
#include <cmath>
#include <QFileInfo>
#include "core/Id.h"
#include "core/Timecode.h"

namespace at {
namespace {
std::string toStd(const QString& s) { const QByteArray b = s.toUtf8(); return std::string(b.constData(), static_cast<size_t>(b.size())); }
QString fromStd(const std::string& s) { return QString::fromUtf8(s.data(), static_cast<int>(s.size())); }
}

Document::Document(QObject* parent) : QObject(parent) { project_ = makeDefaultProject(); }

Layer* Document::selectedLayer() {
    Composition* c = activeComp();
    return c ? c->findLayer(selected_) : nullptr;
}
double Document::fps() { Composition* c = activeComp(); return c ? c->fps.toDouble() : 30.0; }
QString Document::displayName() const { return filePath_.isEmpty() ? QStringLiteral("Untitled") : QFileInfo(filePath_).fileName(); }

double Document::sourceDuration(const Layer& l) const {
    if (l.type != LayerType::Footage) return -1;
    const Footage* f = project_.findFootage(l.footageId);
    if (!f || f->kind != FootageKind::Video || f->duration <= 0) return -1;
    return f->duration;
}

void Document::setDirty(bool d) { if (dirty_ != d) { dirty_ = d; emit titleChanged(); } }
void Document::markChanged() { setDirty(true); emit changed(); }

void Document::resetSession() {
    media_.clear();
    selected_.clear();
    time_ = 0;
}

// ---------------- project lifecycle ----------------
void Document::newProject() {
    project_ = makeDefaultProject();
    filePath_.clear();
    resetSession();
    dirty_ = false;
    emit titleChanged(); emit changed(); emit selectionChanged(); emit timeChanged(0);
}
bool Document::openProject(const QString& path, QString& err) {
    Project p; std::string e;
    if (!loadProjectFile(toStd(path), p, e)) { err = fromStd(e); return false; }
    if (p.comps.empty()) { Composition c = makeComposition("Comp 1", 1920, 1080, Rational{30, 1}, 10); p.activeCompId = c.id; p.comps.push_back(c); }
    project_ = std::move(p);
    filePath_ = path;
    resetSession();
    dirty_ = false;
    emit titleChanged(); emit changed(); emit selectionChanged(); emit timeChanged(0);
    return true;
}
bool Document::saveProject(QString& err) {
    if (filePath_.isEmpty()) { err = "No file name yet."; return false; }
    std::string e;
    if (!saveProjectFile(project_, toStd(filePath_), e)) { err = fromStd(e); return false; }
    setDirty(false);
    return true;
}
bool Document::saveProjectAs(const QString& path, QString& err) {
    std::string e;
    if (!saveProjectFile(project_, toStd(path), e)) { err = fromStd(e); return false; }
    filePath_ = path;
    project_.name = toStd(QFileInfo(path).completeBaseName());
    setDirty(false);
    emit titleChanged();
    return true;
}
bool Document::writeRecoveryCopy(const QString& path, QString& err) {
    std::string e;
    if (!saveProjectFile(project_, toStd(path), e)) { err = fromStd(e); return false; }
    return true;
}
bool Document::openRecoveryCopy(const QString& path, QString& err) {
    Project p; std::string e;
    if (!loadProjectFile(toStd(path), p, e)) { err = fromStd(e); return false; }
    project_ = std::move(p);
    filePath_.clear();
    resetSession();
    dirty_ = true;
    emit titleChanged(); emit changed(); emit selectionChanged(); emit timeChanged(0);
    return true;
}

// ---------------- footage ----------------
bool Document::importFile(const QString& path, QString& err, std::string* idOut) {
    const std::string p = toStd(path);
    for (const Footage& f : project_.footage)
        if (f.path == p) { if (idOut) *idOut = f.id; return true; }   // already imported
    Footage f; std::string e;
    if (!MediaManager::probe(p, f, e)) { err = QStringLiteral("%1\n\n%2").arg(path, fromStd(e)); return false; }
    if (idOut) *idOut = f.id;
    project_.footage.push_back(f);
    markChanged();
    return true;
}
bool Document::removeFootage(const std::string& id) {
    if (project_.footageInUse(id)) return false;
    auto& v = project_.footage;
    v.erase(std::remove_if(v.begin(), v.end(), [&](const Footage& f) { return f.id == id; }), v.end());
    markChanged();
    return true;
}
bool Document::relinkFootage(const std::string& id, const QString& newPath, QString& err) {
    Footage* f = project_.findFootage(id);
    if (!f) { err = "Footage not found."; return false; }
    Footage probed; std::string e;
    if (!MediaManager::probe(toStd(newPath), probed, e)) { err = fromStd(e); return false; }
    f->path = probed.path;
    f->width = probed.width; f->height = probed.height; f->fps = probed.fps; f->duration = probed.duration;
    f->kind = probed.kind; f->hasAudio = probed.hasAudio;
    media_.clear();
    markChanged();
    return true;
}

// ---------------- compositions ----------------
void Document::addComposition(const QString& name, int w, int h, Rational fps, double duration) {
    Composition c = makeComposition(toStd(name), w, h, fps, duration);
    project_.activeCompId = c.id;
    project_.comps.push_back(std::move(c));
    selected_.clear(); time_ = 0;
    markChanged(); emit selectionChanged(); emit timeChanged(0);
}
void Document::updateComposition(const std::string& id, const QString& name, int w, int h, Rational fps, double duration) {
    Composition* c = project_.findComp(id);
    if (!c) return;
    c->name = toStd(name); c->width = w; c->height = h; c->fps = fps; c->duration = duration;
    if (id == project_.activeCompId && time_ > duration) time_ = 0;
    markChanged(); emit timeChanged(time_);
}
void Document::setActiveComp(const std::string& id) {
    if (!project_.findComp(id) || id == project_.activeCompId) return;
    project_.activeCompId = id;
    selected_.clear(); time_ = 0;
    markChanged(); emit selectionChanged(); emit timeChanged(0);
}

// ---------------- layers ----------------
void Document::addFootageLayer(const std::string& footageId) {
    Composition* c = activeComp();
    const Footage* f = project_.findFootage(footageId);
    if (!c || !f || f->kind == FootageKind::Audio) return;
    Layer l = makeFootageLayer(*f, *c, time_);
    selected_ = l.id;
    c->layers.insert(c->layers.begin(), std::move(l));   // new layers go on top
    markChanged(); emit selectionChanged();
}
void Document::addSolidLayer(const QColor& col) {
    Composition* c = activeComp();
    if (!c) return;
    Layer l = makeSolidLayer(toStd(col.name().toUpper()) + " Solid", col.red(), col.green(), col.blue(), *c, time_);
    selected_ = l.id;
    c->layers.insert(c->layers.begin(), std::move(l));
    markChanged(); emit selectionChanged();
}
void Document::setSolidColor(const std::string& id, const QColor& col) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l || l->type != LayerType::Solid) return;
    l->solidR = col.red(); l->solidG = col.green(); l->solidB = col.blue();
    markChanged();
}
void Document::removeLayer(const std::string& id) {
    Composition* c = activeComp(); if (!c) return;
    const int i = c->layerIndex(id); if (i < 0) return;
    c->layers.erase(c->layers.begin() + i);
    if (selected_ == id) { selected_.clear(); emit selectionChanged(); }
    markChanged();
}
void Document::duplicateLayer(const std::string& id) {
    Composition* c = activeComp(); if (!c) return;
    const int i = c->layerIndex(id); if (i < 0) return;
    Layer copy = c->layers[static_cast<size_t>(i)];
    copy.id = newId(); copy.name += " copy";
    selected_ = copy.id;
    c->layers.insert(c->layers.begin() + i, std::move(copy));
    markChanged(); emit selectionChanged();
}
void Document::moveLayer(const std::string& id, int delta) {
    Composition* c = activeComp(); if (!c) return;
    const int i = c->layerIndex(id); if (i < 0) return;
    const int j = std::clamp(i + delta, 0, static_cast<int>(c->layers.size()) - 1);
    if (i == j) return;
    std::swap(c->layers[static_cast<size_t>(i)], c->layers[static_cast<size_t>(j)]);
    markChanged();
}
void Document::splitLayerAtTime(const std::string& id) {
    Composition* c = activeComp(); if (!c) return;
    const int i = c->layerIndex(id); if (i < 0) return;
    Layer& l = c->layers[static_cast<size_t>(i)];
    const double t = time_;
    const double minLen = 1.0 / std::max(1.0, fps());
    if (t <= l.inPoint + minLen * 0.5 || t >= l.outPoint - minLen * 0.5) return;   // playhead must be inside the layer
    Layer second = l;
    second.id = newId();
    second.inPoint = t;
    l.outPoint = t;
    selected_ = second.id;
    c->layers.insert(c->layers.begin() + i, std::move(second));   // second half goes above the first
    markChanged(); emit selectionChanged();
}
void Document::renameLayer(const std::string& id, const QString& name) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l || name.trimmed().isEmpty()) return;
    l->name = toStd(name.trimmed());
    markChanged();
}
void Document::setLayerVisible(const std::string& id, bool v) { Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr; if (l) { l->visible = v; markChanged(); } }
void Document::setLayerSolo(const std::string& id, bool v) { Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr; if (l) { l->solo = v; markChanged(); } }
void Document::setLayerLocked(const std::string& id, bool v) { Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr; if (l) { l->locked = v; markChanged(); } }
void Document::setLayerTiming(const std::string& id, double start, double in, double out) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l) return;
    l->startTime = start; l->inPoint = in; l->outPoint = out;
    markChanged();
}
void Document::fitLayerToComp(const std::string& id) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l) return;
    double w = l->solidW, h = l->solidH;
    if (l->type == LayerType::Footage) {
        const Footage* f = project_.findFootage(l->footageId);
        if (!f) return;
        w = f->width; h = f->height;
    }
    if (w <= 0 || h <= 0) return;
    const double s = std::min(c->width / w, c->height / h) * 100.0;
    for (Prop p : {Prop::ScaleX, Prop::ScaleY}) { l->prop(p).clearKeys(); l->prop(p).setStaticValue(s); }
    for (Prop p : {Prop::PosX}) { l->prop(p).clearKeys(); l->prop(p).setStaticValue(c->width / 2.0); }
    for (Prop p : {Prop::PosY}) { l->prop(p).clearKeys(); l->prop(p).setStaticValue(c->height / 2.0); }
    l->prop(Prop::Rotation).clearKeys(); l->prop(Prop::Rotation).setStaticValue(0);
    l->prop(Prop::AnchorX).clearKeys(); l->prop(Prop::AnchorX).setStaticValue(w / 2.0);
    l->prop(Prop::AnchorY).clearKeys(); l->prop(Prop::AnchorY).setStaticValue(h / 2.0);
    markChanged();
}

// ---------------- properties ----------------
void Document::setPropertyValue(const std::string& id, Prop p, double v) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l) return;
    AnimatedProperty& ap = l->prop(p);
    if (ap.isAnimated()) {
        Interp interp = newKeyInterp_;
        ap.setKey(time_ - l->startTime, v, interp);   // creates a key at the playhead, or edits the existing one
    } else {
        ap.setStaticValue(v);
    }
    markChanged();
}
void Document::setPropertyAnimated(const std::string& id, Prop p, bool animated) {
    Composition* c = activeComp(); Layer* l = c ? c->findLayer(id) : nullptr;
    if (!l) return;
    AnimatedProperty& ap = l->prop(p);
    const double lt = time_ - l->startTime;
    const double cur = ap.valueAt(lt);
    if (animated && !ap.isAnimated()) ap.setKey(lt, cur, newKeyInterp_);
    else if (!animated && ap.isAnimated()) { ap.clearKeys(); ap.setStaticValue(cur); }
    markChanged();
}

// ---------------- navigation ----------------
void Document::setTime(double t) {
    Composition* c = activeComp();
    const double f = fps();
    if (c) t = std::clamp(t, 0.0, std::max(0.0, c->duration - 1.0 / f));
    t = snapToFrame(t, f);
    if (std::fabs(t - time_) < 1e-9) return;
    time_ = t;
    emit timeChanged(time_);
}
void Document::selectLayer(const std::string& id) {
    if (selected_ == id) return;
    selected_ = id;
    emit selectionChanged();
}
}
