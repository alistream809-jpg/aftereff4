#include "ui/TimelinePanel.h"
#include <QAction>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>
#include <iterator>
#include <cmath>
#include "core/Timecode.h"
#include "ui/Document.h"

namespace at {
namespace {
constexpr int kNameW = 230, kRulerH = 26, kRowH = 26, kEdge = 6;
constexpr int kBoxW = 16;   // V S L switches
QRect switchRect(int rowTop, int idx) { return QRect(4 + idx * (kBoxW + 3), rowTop + 5, kBoxW, kRowH - 10); }
}

TimelineView::TimelineView(Document& doc, QWidget* parent) : QWidget(parent), doc_(doc) {
    setMinimumHeight(120);
    setFocusPolicy(Qt::ClickFocus);
    connect(&doc_, &Document::changed, this, [this] {
        Composition* c = doc_.activeComp();
        if (c && c->id != lastComp_) { lastComp_ = c->id; fitToComp(); }
        update();
    });
    connect(&doc_, &Document::timeChanged, this, [this](double) { update(); });
    connect(&doc_, &Document::selectionChanged, this, [this] { update(); });
}

double TimelineView::xToTime(int x) const { return viewStart_ + (x - kNameW) / pps_; }
int TimelineView::timeToX(double t) const { return kNameW + static_cast<int>(std::lround((t - viewStart_) * pps_)); }

void TimelineView::fitToComp() {
    Composition* c = doc_.activeComp();
    if (!c) return;
    pps_ = std::max(2.0, (width() - kNameW - 30) / std::max(0.5, c->duration));
    viewStart_ = 0;
    scrollY_ = 0;
    update();
}

void TimelineView::zoomBy(double f) {
    const double centre = xToTime(kNameW + (width() - kNameW) / 2);
    pps_ = std::clamp(pps_ * f, 1.0, 4000.0);
    viewStart_ = centre - (width() - kNameW) / 2.0 / pps_;
    update();
}

int TimelineView::rowAt(int y) const {
    if (y < kRulerH) return -1;
    return (y - kRulerH + scrollY_) / kRowH;
}

void TimelineView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(30, 30, 34));
    Composition* c = doc_.activeComp();
    if (!c) { p.setPen(Qt::gray); p.drawText(rect(), Qt::AlignCenter, "No composition"); return; }
    const double fps = c->fps.toDouble();
    const QRect track(kNameW, kRulerH, width() - kNameW, height() - kRulerH);

    // ---- track area ----
    p.save();
    p.setClipRect(track);
    const int endX = timeToX(c->duration);
    if (endX < width()) p.fillRect(QRect(std::max(endX, kNameW), kRulerH, width() - endX, height()), QColor(22, 22, 25));
    const int nRows = static_cast<int>(c->layers.size());
    for (int i = 0; i < nRows; ++i) {
        const Layer& L = c->layers[static_cast<size_t>(i)];
        const int top = kRulerH + i * kRowH - scrollY_;
        if (top + kRowH < kRulerH || top > height()) continue;
        const bool sel = L.id == doc_.selectedLayerId();
        p.fillRect(QRect(kNameW, top, width() - kNameW, kRowH), sel ? QColor(52, 60, 78) : (i % 2 ? QColor(34, 34, 38) : QColor(38, 38, 43)));
        const int x0 = timeToX(L.inPoint), x1 = timeToX(L.outPoint);
        QColor col(70, 120, 200);
        if (L.type == LayerType::Solid) col = QColor(L.solidR, L.solidG, L.solidB).darker(130);
        else if (const Footage* f = doc_.project().findFootage(L.footageId)) {
            if (f->kind == FootageKind::Image) col = QColor(70, 150, 100);
        }
        if (!L.visible) col = col.darker(220);
        const QRect bar(x0, top + 3, std::max(2, x1 - x0), kRowH - 6);
        p.fillRect(bar, col);
        p.setPen(sel ? QColor(255, 255, 255) : col.lighter(160));
        p.drawRect(bar.adjusted(0, 0, -1, -1));
        p.setPen(Qt::white);
        p.drawText(bar.adjusted(5, 0, -3, 0), Qt::AlignVCenter | Qt::AlignLeft, QString::fromUtf8(L.name.data(), static_cast<int>(L.name.size())));
        // keyframe diamonds
        p.setBrush(QColor(255, 210, 80)); p.setPen(Qt::black);
        for (int k = 0; k < kPropCount; ++k)
            for (const Keyframe& kf : L.props[k].keys()) {
                const int kx = timeToX(L.startTime + kf.time);
                if (kx < x0 - 3 || kx > x1 + 3) continue;
                const int cy = top + kRowH - 7;
                const QPoint pts[4] = {{kx, cy - 4}, {kx + 4, cy}, {kx, cy + 4}, {kx - 4, cy}};
                p.drawPolygon(pts, 4);
            }
        p.setBrush(Qt::NoBrush);
    }
    p.restore();

    // ---- name column ----
    p.fillRect(QRect(0, kRulerH, kNameW, height()), QColor(36, 36, 40));
    p.save();
    p.setClipRect(QRect(0, kRulerH, kNameW, height() - kRulerH));
    for (int i = 0; i < nRows; ++i) {
        const Layer& L = c->layers[static_cast<size_t>(i)];
        const int top = kRulerH + i * kRowH - scrollY_;
        if (top + kRowH < kRulerH || top > height()) continue;
        const bool sel = L.id == doc_.selectedLayerId();
        if (sel) p.fillRect(QRect(0, top, kNameW, kRowH), QColor(52, 60, 78));
        const bool flags[3] = {L.visible, L.solo, L.locked};
        const char* letters[3] = {"V", "S", "L"};
        for (int s = 0; s < 3; ++s) {
            const QRect r = switchRect(top, s);
            p.fillRect(r, flags[s] ? (s == 1 ? QColor(220, 170, 50) : QColor(80, 140, 220)) : QColor(55, 55, 60));
            p.setPen(flags[s] ? Qt::black : QColor(130, 130, 135));
            p.drawText(r, Qt::AlignCenter, letters[s]);
        }
        p.setPen(L.visible ? QColor(225, 225, 230) : QColor(120, 120, 125));
        p.drawText(QRect(4 + 3 * (kBoxW + 3) + 6, top, kNameW - 70, kRowH), Qt::AlignVCenter | Qt::AlignLeft,
                   QString("%1  %2").arg(nRows - i).arg(QString::fromUtf8(L.name.data(), static_cast<int>(L.name.size()))));
        p.setPen(QColor(50, 50, 55));
        p.drawLine(0, top + kRowH - 1, kNameW, top + kRowH - 1);
    }
    p.restore();
    p.setPen(QColor(60, 60, 66));
    p.drawLine(kNameW, kRulerH, kNameW, height());

    // ---- ruler ----
    p.fillRect(QRect(0, 0, width(), kRulerH), QColor(44, 44, 50));
    p.setPen(QColor(200, 200, 205));
    p.drawText(QRect(6, 0, kNameW - 8, kRulerH), Qt::AlignVCenter, QString::fromStdString(formatTimecode(doc_.time(), fps)));
    p.save();
    p.setClipRect(QRect(kNameW, 0, width() - kNameW, kRulerH));
    static const double steps[] = {1.0 / 30, 0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 1800};
    double step = steps[std::size(steps) - 1];
    for (double s : steps) if (s * pps_ >= 70) { step = s; break; }
    const double firstT = std::floor(viewStart_ / step) * step;
    for (double t = firstT; t < xToTime(width()) + step; t += step) {
        const int x = timeToX(t);
        p.setPen(QColor(120, 120, 128));
        p.drawLine(x, kRulerH - 10, x, kRulerH);
        for (int m = 1; m < 5; ++m) { const int mx = timeToX(t + step * m / 5.0); p.drawLine(mx, kRulerH - 4, mx, kRulerH); }
        p.setPen(QColor(190, 190, 195));
        p.drawText(x + 3, 12, QString::fromStdString(formatTimecode(t, fps)));
    }
    p.restore();

    // ---- playhead ----
    const int px = timeToX(doc_.time());
    if (px >= kNameW && px <= width()) {
        p.setPen(QPen(QColor(235, 70, 70), 1));
        p.drawLine(px, 0, px, height());
        const QPoint head[3] = {{px - 6, 0}, {px + 6, 0}, {px, 10}};
        p.setBrush(QColor(235, 70, 70)); p.setPen(Qt::NoPen);
        p.drawPolygon(head, 3);
    }
}

void TimelineView::showEvent(QShowEvent*) {
    if (!shownOnce_) { shownOnce_ = true; QTimer::singleShot(0, this, [this] { fitToComp(); }); }
}

void TimelineView::scrubTo(int x) {
    doc_.setTime(xToTime(std::max(x, kNameW)));
}

void TimelineView::mousePressEvent(QMouseEvent* e) {
    setFocus();
    Composition* c = doc_.activeComp();
    if (!c || e->button() != Qt::LeftButton) return;
    const int x = static_cast<int>(e->position().x()), y = static_cast<int>(e->position().y());
    if (y < kRulerH) { if (x >= kNameW) { drag_ = Drag::Scrub; scrubTo(x); } return; }
    const int row = rowAt(y);
    if (row < 0 || row >= static_cast<int>(c->layers.size())) { doc_.selectLayer(""); return; }
    const Layer& L = c->layers[static_cast<size_t>(row)];
    const int top = kRulerH + row * kRowH - scrollY_;
    if (x < kNameW) {
        if (switchRect(top, 0).contains(x, y)) doc_.setLayerVisible(L.id, !L.visible);
        else if (switchRect(top, 1).contains(x, y)) doc_.setLayerSolo(L.id, !L.solo);
        else if (switchRect(top, 2).contains(x, y)) doc_.setLayerLocked(L.id, !L.locked);
        else doc_.selectLayer(L.id);
        return;
    }
    doc_.selectLayer(L.id);
    const int x0 = timeToX(L.inPoint), x1 = timeToX(L.outPoint);
    if (L.locked || x < x0 - 2 || x > x1 + 2) return;
    dragId_ = L.id; pressX_ = x;
    origStart_ = L.startTime; origIn_ = L.inPoint; origOut_ = L.outPoint;
    drag_ = (x <= x0 + kEdge) ? Drag::TrimIn : (x >= x1 - kEdge ? Drag::TrimOut : Drag::Move);
}

void TimelineView::mouseMoveEvent(QMouseEvent* e) {
    Composition* c = doc_.activeComp();
    if (!c) return;
    const int x = static_cast<int>(e->position().x());
    if (drag_ == Drag::None) {
        const int row = rowAt(static_cast<int>(e->position().y()));
        Qt::CursorShape cs = Qt::ArrowCursor;
        if (row >= 0 && row < static_cast<int>(c->layers.size()) && x >= kNameW) {
            const Layer& L = c->layers[static_cast<size_t>(row)];
            const int x0 = timeToX(L.inPoint), x1 = timeToX(L.outPoint);
            if (x >= x0 - 2 && x <= x1 + 2) cs = (x <= x0 + kEdge || x >= x1 - kEdge) ? Qt::SizeHorCursor : Qt::OpenHandCursor;
        }
        setCursor(cs);
        return;
    }
    if (drag_ == Drag::Scrub) { scrubTo(x); return; }
    const Layer* L = c->findLayer(dragId_);
    if (!L) return;
    const double fps = c->fps.toDouble();
    const double dt = snapToFrame((x - pressX_) / pps_, fps);
    const double minLen = 1.0 / fps;
    const double srcDur = doc_.sourceDuration(*L);
    if (drag_ == Drag::Move) {
        doc_.setLayerTiming(dragId_, origStart_ + dt, origIn_ + dt, origOut_ + dt);
    } else if (drag_ == Drag::TrimIn) {
        double nin = origIn_ + dt;
        if (srcDur > 0) nin = std::max(nin, origStart_);
        nin = std::min(nin, origOut_ - minLen);
        doc_.setLayerTiming(dragId_, origStart_, nin, origOut_);
    } else if (drag_ == Drag::TrimOut) {
        double nout = origOut_ + dt;
        if (srcDur > 0) nout = std::min(nout, origStart_ + srcDur);
        nout = std::max(nout, origIn_ + minLen);
        doc_.setLayerTiming(dragId_, origStart_, origIn_, nout);
    }
}

void TimelineView::mouseReleaseEvent(QMouseEvent*) { drag_ = Drag::None; dragId_.clear(); }

void TimelineView::mouseDoubleClickEvent(QMouseEvent* e) {
    Composition* c = doc_.activeComp();
    if (!c) return;
    const int x = static_cast<int>(e->position().x());
    const int row = rowAt(static_cast<int>(e->position().y()));
    if (x >= kNameW || row < 0 || row >= static_cast<int>(c->layers.size())) return;
    const Layer& L = c->layers[static_cast<size_t>(row)];
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Rename layer", "Layer name:", QLineEdit::Normal,
                                               QString::fromUtf8(L.name.data(), static_cast<int>(L.name.size())), &ok);
    if (ok) doc_.renameLayer(L.id, name);
}

void TimelineView::wheelEvent(QWheelEvent* e) {
    const bool ctrl = e->modifiers() & Qt::ControlModifier;
    const bool shift = e->modifiers() & Qt::ShiftModifier;
    const int dy = e->angleDelta().y(), dx = e->angleDelta().x();
    if (ctrl) {
        const double factor = dy > 0 ? 1.25 : 0.8;
        const int mx = static_cast<int>(e->position().x());
        const double tAt = xToTime(mx);
        pps_ = std::clamp(pps_ * factor, 1.0, 4000.0);
        viewStart_ = tAt - (mx - kNameW) / pps_;
    } else if (shift || dx != 0) {
        viewStart_ -= (dx != 0 ? dx : dy) / 120.0 * 60.0 / pps_;
    } else {
        Composition* c = doc_.activeComp();
        const int maxScroll = c ? std::max(0, static_cast<int>(c->layers.size()) * kRowH - (height() - kRulerH)) : 0;
        scrollY_ = std::clamp(scrollY_ - dy / 120 * kRowH, 0, maxScroll);
    }
    update();
}

void TimelineView::contextMenuEvent(QContextMenuEvent* e) {
    Composition* c = doc_.activeComp();
    if (!c) return;
    const int row = rowAt(e->pos().y());
    if (row >= 0 && row < static_cast<int>(c->layers.size())) doc_.selectLayer(c->layers[static_cast<size_t>(row)].id);
    QMenu m(this);
    for (QAction* a : actions_) { if (a) m.addAction(a); else m.addSeparator(); }
    m.exec(e->globalPos());
}
}
