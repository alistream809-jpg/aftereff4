#pragma once
#include <QColor>
#include <QObject>
#include <QString>
#include <string>
#include "media/MediaManager.h"
#include "project/Project.h"

namespace at {

// The single source of truth for the open project. All panels read from it and
// call its edit methods; it emits signals so every panel stays in sync.
class Document : public QObject {
    Q_OBJECT
public:
    explicit Document(QObject* parent = nullptr);

    Project& project() { return project_; }
    const Project& project() const { return project_; }
    MediaManager& media() { return media_; }
    Composition* activeComp() { return project_.findComp(project_.activeCompId); }
    Layer* selectedLayer();
    const std::string& selectedLayerId() const { return selected_; }

    const QString& filePath() const { return filePath_; }
    bool hasFilePath() const { return !filePath_.isEmpty(); }
    QString displayName() const;
    bool isDirty() const { return dirty_; }
    double time() const { return time_; }
    double fps();
    // Maximum usable length of a layer's source in seconds (-1 = unlimited, e.g. stills/solids).
    double sourceDuration(const Layer& l) const;

    // Project lifecycle
    void newProject();
    bool openProject(const QString& path, QString& err);
    bool saveProject(QString& err);
    bool saveProjectAs(const QString& path, QString& err);
    bool writeRecoveryCopy(const QString& path, QString& err);
    bool openRecoveryCopy(const QString& path, QString& err);

    // Footage
    bool importFile(const QString& path, QString& err, std::string* idOut = nullptr);
    bool removeFootage(const std::string& id);
    bool relinkFootage(const std::string& id, const QString& newPath, QString& err);

    // Compositions
    void addComposition(const QString& name, int w, int h, Rational fps, double duration);
    void updateComposition(const std::string& id, const QString& name, int w, int h, Rational fps, double duration);
    void setActiveComp(const std::string& id);

    // Layers
    void addFootageLayer(const std::string& footageId);
    void addSolidLayer(const QColor& c);
    void setSolidColor(const std::string& layerId, const QColor& c);
    void removeLayer(const std::string& id);
    void duplicateLayer(const std::string& id);
    void moveLayer(const std::string& id, int delta);   // -1 = up (toward top), +1 = down
    void splitLayerAtTime(const std::string& id);
    void renameLayer(const std::string& id, const QString& name);
    void setLayerVisible(const std::string& id, bool v);
    void setLayerSolo(const std::string& id, bool v);
    void setLayerLocked(const std::string& id, bool v);
    void setLayerTiming(const std::string& id, double start, double in, double out);
    void fitLayerToComp(const std::string& id);

    // Properties & animation
    void setPropertyValue(const std::string& layerId, Prop p, double value);   // keys if animated
    void setPropertyAnimated(const std::string& layerId, Prop p, bool animated);
    void setNewKeyInterp(Interp i) { newKeyInterp_ = i; }
    Interp newKeyInterp() const { return newKeyInterp_; }

    // Navigation
    void setTime(double t);
    void selectLayer(const std::string& id);

signals:
    void changed();                 // anything in the project changed
    void timeChanged(double t);
    void selectionChanged();
    void titleChanged();            // file name or dirty flag changed

private:
    void markChanged();
    void setDirty(bool d);
    void resetSession();
    Project project_;
    MediaManager media_;
    QString filePath_;
    bool dirty_ = false;
    double time_ = 0;
    std::string selected_;
    Interp newKeyInterp_ = Interp::Linear;
};
}
