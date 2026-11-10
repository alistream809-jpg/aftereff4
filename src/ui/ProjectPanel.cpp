#include "ui/ProjectPanel.h"
#include <QFileDialog>
#include <QHeaderView>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include "core/FileUtil.h"
#include "core/Timecode.h"
#include "ui/Document.h"

namespace at {
namespace {
enum Roles { RoleId = Qt::UserRole, RoleType = Qt::UserRole + 1 };   // type: 1 = comp, 2 = footage
QString fromStd(const std::string& s) { return QString::fromUtf8(s.data(), static_cast<int>(s.size())); }
}

ProjectPanel::ProjectPanel(Document& doc, QWidget* parent) : QWidget(parent), doc_(doc) {
    tree_ = new QTreeWidget;
    tree_->setHeaderHidden(true);
    tree_->setIconSize(QSize(64, 40));
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setUniformRowHeights(false);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addWidget(tree_);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, &ProjectPanel::onDoubleClicked);
    connect(tree_, &QTreeWidget::customContextMenuRequested, this, &ProjectPanel::onContextMenu);
    connect(&doc_, &Document::changed, this, &ProjectPanel::rebuild);
    rebuild();
}

QIcon ProjectPanel::thumbnailFor(const std::string& id, const std::string& path) {
    const QString key = fromStd(id) + "|" + fromStd(path);
    auto it = thumbs_.find(key);
    if (it != thumbs_.end()) return it.value();
    QIcon icon;
    Frame f;
    if (fileExists(path) && MediaManager::grabPreviewFrame(path, f) && !f.empty()) {
        const QImage img(f.rgba.data(), f.width, f.height, f.width * 4, QImage::Format_RGBA8888);
        icon = QIcon(QPixmap::fromImage(img.scaled(64, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
    thumbs_.insert(key, icon);
    return icon;
}

void ProjectPanel::rebuild() {
    const QString selId = tree_->currentItem() ? tree_->currentItem()->data(0, RoleId).toString() : QString();
    tree_->clear();
    auto* comps = new QTreeWidgetItem(tree_, QStringList("Compositions"));
    auto* foot = new QTreeWidgetItem(tree_, QStringList("Footage"));
    for (auto* g : {comps, foot}) { QFont f = g->font(0); f.setBold(true); g->setFont(0, f); g->setFlags(Qt::ItemIsEnabled); }

    for (const Composition& c : doc_.project().comps) {
        auto* it = new QTreeWidgetItem(comps);
        QString txt = fromStd(c.name) + QString("\n%1 x %2, %3 fps, %4")
                          .arg(c.width).arg(c.height).arg(c.fps.toDouble(), 0, 'g', 5)
                          .arg(fromStd(formatTimecode(c.duration, c.fps.toDouble())));
        if (c.id == doc_.project().activeCompId) txt = QString::fromUtf8("\u25B6 ") + txt;
        it->setText(0, txt);
        it->setData(0, RoleId, fromStd(c.id)); it->setData(0, RoleType, 1);
    }
    for (const Footage& f : doc_.project().footage) {
        auto* it = new QTreeWidgetItem(foot);
        const bool missing = !fileExists(f.path);
        QString info;
        if (f.kind == FootageKind::Audio) info = QString("Audio, %1").arg(fromStd(formatTimecode(f.duration, 30)));
        else if (f.kind == FootageKind::Image) info = QString("Image, %1 x %2").arg(f.width).arg(f.height);
        else info = QString("Video, %1 x %2, %3 fps, %4").arg(f.width).arg(f.height).arg(f.fps, 0, 'g', 5).arg(fromStd(formatTimecode(f.duration, f.fps)));
        if (f.kind == FootageKind::Audio) info += "  (audio playback not available in this version)";
        it->setText(0, fromStd(f.name) + "\n" + info + (missing ? "\nMISSING FILE - right-click > Relink" : ""));
        if (missing) it->setForeground(0, QColor(230, 90, 90));
        else if (f.kind != FootageKind::Audio) it->setIcon(0, thumbnailFor(f.id, f.path));
        it->setToolTip(0, fromStd(f.path));
        it->setData(0, RoleId, fromStd(f.id)); it->setData(0, RoleType, 2);
    }
    tree_->expandAll();
    if (!selId.isEmpty()) {
        QTreeWidgetItemIterator iter(tree_);
        while (*iter) { if ((*iter)->data(0, RoleId).toString() == selId) { tree_->setCurrentItem(*iter); break; } ++iter; }
    }
}

void ProjectPanel::onDoubleClicked(QTreeWidgetItem* item) {
    if (!item) return;
    const std::string id = item->data(0, RoleId).toString().toStdString();
    const int type = item->data(0, RoleType).toInt();
    if (type == 1) doc_.setActiveComp(id);
    else if (type == 2) doc_.addFootageLayer(id);
}

void ProjectPanel::onContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = tree_->itemAt(pos);
    QMenu menu(this);
    const int type = item ? item->data(0, RoleType).toInt() : 0;
    const std::string id = item ? item->data(0, RoleId).toString().toStdString() : std::string();
    if (type == 1) {
        menu.addAction("Open composition", [this, id] { doc_.setActiveComp(id); });
    } else if (type == 2) {
        const Footage* f = doc_.project().findFootage(id);
        if (f && f->kind != FootageKind::Audio)
            menu.addAction("Add to active composition", [this, id] { doc_.addFootageLayer(id); });
        menu.addAction("Relink / replace file...", [this, id] {
            const QString p = QFileDialog::getOpenFileName(this, "Choose the file for this footage");
            if (p.isEmpty()) return;
            QString err;
            if (!doc_.relinkFootage(id, p, err)) QMessageBox::warning(this, "Relink failed", err);
        });
        menu.addAction("Remove from project", [this, id] {
            if (!doc_.removeFootage(id))
                QMessageBox::information(this, "Footage in use", "This footage is used by a layer. Delete the layer first.");
        });
    }
    menu.addSeparator();
    menu.addAction("Import footage...", [this] { emit importRequested(); });
    menu.exec(tree_->viewport()->mapToGlobal(pos));
}
}
