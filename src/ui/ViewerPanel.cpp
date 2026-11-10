#include "ui/ViewerPanel.h"
#include <QComboBox>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QSlider>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include "comp/Compositor.h"
#include "core/Timecode.h"
#include "ui/Document.h"

namespace at {

ViewerWidget::ViewerWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ViewerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(18, 18, 20));
    if (frame_.empty()) {
        p.setPen(QColor(150, 150, 155));
        p.drawText(rect(), Qt::AlignCenter, message_);
        return;
    }
    const QImage img(frame_.rgba.data(), frame_.width, frame_.height, frame_.width * 4, QImage::Format_RGBX8888);
    const QSize target = img.size().scaled(size() - QSize(16, 16), Qt::KeepAspectRatio);
    const QRect r(QPoint((width() - target.width()) / 2, (height() - target.height()) / 2), target);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(r, img);
    p.setPen(QColor(70, 70, 75));
    p.drawRect(r.adjusted(-1, -1, 0, 0));
}

ViewerPanel::ViewerPanel(Document& doc, QWidget* parent) : QWidget(parent), doc_(doc) {
    view_ = new ViewerWidget;
    slider_ = new QSlider(Qt::Horizontal);
    timeLabel_ = new QLabel;
    timeLabel_->setMinimumWidth(100);
    timeLabel_->setFont(QFont("Consolas", 10));
    quality_ = new QComboBox;
    quality_->addItem("Full", 1); quality_->addItem("Half", 2); quality_->addItem("Third", 3); quality_->addItem("Quarter", 4);
    quality_->setCurrentIndex(1);
    quality_->setToolTip("Preview resolution. Lower is faster. Exports always use full resolution.");

    auto mkBtn = [&](QStyle::StandardPixmap icon, const QString& tip) {
        auto* b = new QToolButton; b->setIcon(style()->standardIcon(icon)); b->setToolTip(tip); return b;
    };
    auto* toStart = mkBtn(QStyle::SP_MediaSkipBackward, "Go to start (Home)");
    auto* prev = mkBtn(QStyle::SP_MediaSeekBackward, "Previous frame (Left)");
    playBtn_ = mkBtn(QStyle::SP_MediaPlay, "Play / Pause (Space)");
    auto* next = mkBtn(QStyle::SP_MediaSeekForward, "Next frame (Right)");
    auto* toEnd = mkBtn(QStyle::SP_MediaSkipForward, "Go to end (End)");

    auto* bar = new QHBoxLayout;
    bar->addWidget(toStart); bar->addWidget(prev); bar->addWidget(playBtn_); bar->addWidget(next); bar->addWidget(toEnd);
    bar->addWidget(timeLabel_);
    bar->addWidget(slider_, 1);
    bar->addWidget(new QLabel("Preview:"));
    bar->addWidget(quality_);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(view_, 1);
    lay->addLayout(bar);

    connect(toStart, &QToolButton::clicked, this, [this] { doc_.setTime(0); });
    connect(toEnd, &QToolButton::clicked, this, [this] { doc_.setTime(1e9); });
    connect(prev, &QToolButton::clicked, this, [this] { doc_.setTime(doc_.time() - 1.0 / doc_.fps()); });
    connect(next, &QToolButton::clicked, this, [this] { doc_.setTime(doc_.time() + 1.0 / doc_.fps()); });
    connect(playBtn_, &QToolButton::clicked, this, &ViewerPanel::togglePlay);
    connect(slider_, &QSlider::valueChanged, this, [this](int frameIdx) { doc_.setTime(frameIdx / doc_.fps()); });
    connect(quality_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { refresh(); });
    connect(&doc_, &Document::changed, this, &ViewerPanel::refresh);
    connect(&doc_, &Document::timeChanged, this, &ViewerPanel::onTime);
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(4);
    connect(&timer_, &QTimer::timeout, this, &ViewerPanel::onTick);
    refresh();
}

void ViewerPanel::updateTimeLabel() {
    timeLabel_->setText(QString::fromStdString(formatTimecode(doc_.time(), doc_.fps())));
}

void ViewerPanel::onTime(double) {
    const QSignalBlocker b(slider_);
    slider_->setValue(static_cast<int>(std::lround(doc_.time() * doc_.fps())));
    updateTimeLabel();
    refresh();
}

void ViewerPanel::refresh() {
    Composition* c = doc_.activeComp();
    if (!c) { view_->frame() = Frame{}; view_->setMessage("No composition. Use Composition > New Composition."); view_->update(); return; }
    {
        const QSignalBlocker b(slider_);
        slider_->setRange(0, std::max(0, static_cast<int>(std::lround(c->duration * c->fps.toDouble())) - 1));
        slider_->setValue(static_cast<int>(std::lround(doc_.time() * c->fps.toDouble())));
    }
    updateTimeLabel();
    QElapsedTimer t; t.start();
    renderComposition(doc_.project(), *c, doc_.time(), doc_.media(), view_->frame(), quality_->currentData().toInt());
    const qint64 ms = t.elapsed();
    view_->update();
    emit renderStats(QString("%1 x %2 comp | preview %3 x %4 | last frame rendered in %5 ms")
                         .arg(c->width).arg(c->height).arg(view_->frame().width).arg(view_->frame().height).arg(ms));
}

void ViewerPanel::renderFull(Frame& out) {
    Composition* c = doc_.activeComp();
    if (!c) { out = Frame{}; return; }
    renderComposition(doc_.project(), *c, doc_.time(), doc_.media(), out, 1);
}

void ViewerPanel::togglePlay() {
    if (playing_) { stop(); return; }
    Composition* c = doc_.activeComp();
    if (!c) return;
    if (doc_.time() >= c->duration - 2.0 / doc_.fps()) doc_.setTime(0);
    playing_ = true;
    playStart_ = doc_.time();
    clock_.start();
    timer_.start();
    playBtn_->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
}

void ViewerPanel::stop() {
    if (!playing_) return;
    playing_ = false;
    timer_.stop();
    playBtn_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
}

void ViewerPanel::onTick() {
    Composition* c = doc_.activeComp();
    if (!c) { stop(); return; }
    double t = playStart_ + clock_.nsecsElapsed() / 1e9;
    if (t >= c->duration) {           // loop
        playStart_ = 0; clock_.restart(); t = 0;
    }
    doc_.setTime(t);
}
}
