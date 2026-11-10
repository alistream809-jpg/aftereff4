#pragma once
#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>
#include "core/Frame.h"
class QLabel; class QSlider; class QComboBox; class QToolButton;
namespace at {
class Document;

class ViewerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ViewerWidget(QWidget* parent = nullptr);
    void setMessage(const QString& m) { message_ = m; update(); }
    Frame& frame() { return frame_; }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    Frame frame_;
    QString message_;
};

// Composition viewer + transport controls.
class ViewerPanel : public QWidget {
    Q_OBJECT
public:
    explicit ViewerPanel(Document& doc, QWidget* parent = nullptr);
    bool isPlaying() const { return playing_; }
    void togglePlay();
    void stop();
    void renderFull(Frame& out);    // full-resolution render of the current frame (for PNG export)
signals:
    void renderStats(const QString& text);
private slots:
    void refresh();
    void onTime(double t);
    void onTick();
private:
    void updateTimeLabel();
    Document& doc_;
    ViewerWidget* view_;
    QSlider* slider_;
    QLabel* timeLabel_;
    QComboBox* quality_;
    QToolButton* playBtn_;
    QTimer timer_;
    QElapsedTimer clock_;
    bool playing_ = false;
    double playStart_ = 0;
};
}
