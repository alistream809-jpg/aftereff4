#pragma once
#include <QList>
#include <QWidget>
#include <string>
class QAction;
namespace at {
class Document;

// Custom-painted timeline: ruler, playhead, one row per layer (top layer first).
//  - drag in the ruler to move the playhead
//  - drag a layer bar to move it, drag its left/right edge to trim
//  - V / S / L boxes toggle visibility / solo / lock
//  - mouse wheel: scroll rows | Ctrl+wheel: zoom | Shift+wheel: scroll time
class TimelineView : public QWidget {
    Q_OBJECT
public:
    explicit TimelineView(Document& doc, QWidget* parent = nullptr);
    void setLayerActions(const QList<QAction*>& a) { actions_ = a; }
    void zoomBy(double factor);
    void fitToComp();
protected:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void contextMenuEvent(QContextMenuEvent*) override;
    QSize sizeHint() const override { return QSize(900, 240); }
private:
    enum class Drag { None, Scrub, Move, TrimIn, TrimOut };
    int rowAt(int y) const;
    double xToTime(int x) const;
    int timeToX(double t) const;
    void scrubTo(int x);
    Document& doc_;
    double pps_ = 80;          // pixels per second
    double viewStart_ = 0;     // time at the left edge of the track area
    int scrollY_ = 0;
    Drag drag_ = Drag::None;
    std::string dragId_;
    int pressX_ = 0;
    double origStart_ = 0, origIn_ = 0, origOut_ = 0;
    std::string lastComp_;
    bool shownOnce_ = false;
    QList<QAction*> actions_;
};
}
