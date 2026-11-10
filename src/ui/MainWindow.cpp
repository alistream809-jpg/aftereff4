#include "ui/MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QLineEdit>
#include <QDir>
#include <QImage>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QUrl>
#include "ui/CompSettingsDialog.h"
#include "ui/Document.h"
#include "ui/ProjectPanel.h"
#include "ui/PropertiesPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ViewerPanel.h"

#ifndef AT_VERSION
#define AT_VERSION "0.1.0"
#endif

namespace at {

MainWindow::MainWindow() {
    doc_ = new Document(this);
    viewer_ = new ViewerPanel(*doc_);
    setCentralWidget(viewer_);

    projectPanel_ = new ProjectPanel(*doc_);
    projectDock_ = new QDockWidget("Project", this);
    projectDock_->setObjectName("dockProject");
    projectDock_->setWidget(projectPanel_);
    addDockWidget(Qt::LeftDockWidgetArea, projectDock_);

    props_ = new PropertiesPanel(*doc_);
    propsDock_ = new QDockWidget("Properties", this);
    propsDock_->setObjectName("dockProps");
    propsDock_->setWidget(props_);
    addDockWidget(Qt::RightDockWidgetArea, propsDock_);

    timeline_ = new TimelineView(*doc_);
    timelineDock_ = new QDockWidget("Timeline", this);
    timelineDock_->setObjectName("dockTimeline");
    timelineDock_->setWidget(timeline_);
    addDockWidget(Qt::BottomDockWidgetArea, timelineDock_);

    resizeDocks({projectDock_, propsDock_}, {320, 300}, Qt::Horizontal);
    resizeDocks({timelineDock_}, {300}, Qt::Vertical);

    status_ = new QLabel;
    statusBar()->addWidget(status_, 1);
    connect(viewer_, &ViewerPanel::renderStats, status_, &QLabel::setText);

    buildActions();
    buildMenus();
    timeline_->setLayerActions({aNewSolid_, nullptr, aDuplicate_, aSplit_, aRename_, nullptr, aUp_, aDown_, aFit_, nullptr, aDelete_});

    connect(projectPanel_, &ProjectPanel::importRequested, this, &MainWindow::doImport);
    connect(doc_, &Document::titleChanged, this, &MainWindow::updateTitle);
    connect(doc_, &Document::changed, this, &MainWindow::updateActionStates);
    connect(doc_, &Document::selectionChanged, this, &MainWindow::updateActionStates);

    setAcceptDrops(true);
    resize(1500, 900);
    setMinimumSize(900, 600);
    defaultState_ = saveState();
    QSettings s("AfterTrue", "AfterTrue");
    if (s.contains("geometry")) restoreGeometry(s.value("geometry").toByteArray());
    if (s.contains("windowState")) restoreState(s.value("windowState").toByteArray());

    autosaveTimer_.setInterval(60 * 1000);
    connect(&autosaveTimer_, &QTimer::timeout, this, &MainWindow::autosave);
    autosaveTimer_.start();
    updateTitle();
    updateActionStates();
    timeline_->fitToComp();
}

void MainWindow::buildActions() {
    auto mk = [this](const QString& text, const QKeySequence& key, auto slot) {
        auto* a = new QAction(text, this);
        if (!key.isEmpty()) a->setShortcut(key);
        connect(a, &QAction::triggered, this, slot);
        addAction(a);   // makes shortcuts work even when menus are hidden
        return a;
    };
    aNew_ = mk("New Project", QKeySequence("Ctrl+Alt+N"), [this] { doNewProject(); });
    aOpen_ = mk("Open Project...", QKeySequence::Open, [this] { doOpen(); });
    aSave_ = mk("Save", QKeySequence::Save, [this] { doSave(); });
    aSaveAs_ = mk("Save As...", QKeySequence("Ctrl+Shift+S"), [this] { doSaveAs(); });
    aImport_ = mk("Import Footage...", QKeySequence("Ctrl+I"), [this] { doImport(); });
    aExportFrame_ = mk("Export Current Frame as PNG...", QKeySequence("Ctrl+Alt+S"), [this] { doExportFrame(); });
    aQuit_ = mk("Exit", QKeySequence("Ctrl+Q"), [this] { close(); });

    aNewComp_ = mk("New Composition...", QKeySequence("Ctrl+N"), [this] { doNewComp(); });
    aCompSettings_ = mk("Composition Settings...", QKeySequence("Ctrl+K"), [this] { doCompSettings(); });

    aNewSolid_ = mk("New Solid...", QKeySequence("Ctrl+Y"), [this] { doNewSolid(); });
    aDuplicate_ = mk("Duplicate Layer", QKeySequence("Ctrl+D"), [this] { doc_->duplicateLayer(doc_->selectedLayerId()); });
    aDelete_ = mk("Delete Layer", QKeySequence::Delete, [this] { doc_->removeLayer(doc_->selectedLayerId()); });
    aSplit_ = mk("Split Layer at Playhead", QKeySequence("Ctrl+Shift+D"), [this] { doc_->splitLayerAtTime(doc_->selectedLayerId()); });
    aUp_ = mk("Move Layer Up", QKeySequence("Ctrl+]"), [this] { doc_->moveLayer(doc_->selectedLayerId(), -1); });
    aDown_ = mk("Move Layer Down", QKeySequence("Ctrl+["), [this] { doc_->moveLayer(doc_->selectedLayerId(), +1); });
    aFit_ = mk("Fit Layer to Composition", QKeySequence("Ctrl+Alt+F"), [this] { doc_->fitLayerToComp(doc_->selectedLayerId()); });
    aRename_ = mk("Rename Layer...", QKeySequence("F2"), [this] {
        const Layer* l = doc_->selectedLayer();
        if (!l) return;
        bool ok = false;
        const QString n = QInputDialog::getText(this, "Rename layer", "Layer name:", QLineEdit::Normal, QString::fromStdString(l->name), &ok);
        if (ok) doc_->renameLayer(l->id, n);
    });

    aPlay_ = mk("Play / Pause", QKeySequence(Qt::Key_Space), [this] { viewer_->togglePlay(); });
    aStart_ = mk("Go to Start", QKeySequence(Qt::Key_Home), [this] { doc_->setTime(0); });
    aEnd_ = mk("Go to End", QKeySequence(Qt::Key_End), [this] { doc_->setTime(1e9); });
    aNext_ = mk("Next Frame", QKeySequence(Qt::Key_Right), [this] { doc_->setTime(doc_->time() + 1.0 / doc_->fps()); });
    aPrev_ = mk("Previous Frame", QKeySequence(Qt::Key_Left), [this] { doc_->setTime(doc_->time() - 1.0 / doc_->fps()); });

    aZoomIn_ = mk("Zoom Timeline In", QKeySequence(Qt::Key_Equal), [this] { timeline_->zoomBy(1.25); });
    aZoomOut_ = mk("Zoom Timeline Out", QKeySequence(Qt::Key_Minus), [this] { timeline_->zoomBy(0.8); });
    aZoomFit_ = mk("Fit Timeline to Composition", QKeySequence(Qt::Key_Semicolon), [this] { timeline_->fitToComp(); });
    aResetLayout_ = mk("Reset Panel Layout", QKeySequence(), [this] { resetLayout(); });
    aAbout_ = mk("About AFTER TRUE / Feature Status", QKeySequence(), [this] { doAbout(); });
}

void MainWindow::buildMenus() {
    QMenu* file = menuBar()->addMenu("&File");
    file->addAction(aNew_); file->addAction(aOpen_);
    recentMenu_ = file->addMenu("Open Recent");
    file->addSeparator();
    file->addAction(aSave_); file->addAction(aSaveAs_);
    file->addSeparator();
    file->addAction(aImport_); file->addAction(aExportFrame_);
    file->addSeparator();
    file->addAction(aQuit_);
    rebuildRecentMenu();

    QMenu* comp = menuBar()->addMenu("&Composition");
    comp->addAction(aNewComp_); comp->addAction(aCompSettings_);

    QMenu* layer = menuBar()->addMenu("&Layer");
    layer->addAction(aNewSolid_);
    layer->addSeparator();
    layer->addAction(aDuplicate_); layer->addAction(aSplit_); layer->addAction(aRename_);
    layer->addSeparator();
    layer->addAction(aUp_); layer->addAction(aDown_); layer->addAction(aFit_);
    layer->addSeparator();
    layer->addAction(aDelete_);

    QMenu* play = menuBar()->addMenu("&Playback");
    play->addAction(aPlay_); play->addAction(aStart_); play->addAction(aEnd_);
    play->addAction(aPrev_); play->addAction(aNext_);

    QMenu* view = menuBar()->addMenu("&View");
    view->addAction(aZoomIn_); view->addAction(aZoomOut_); view->addAction(aZoomFit_);
    view->addSeparator();
    view->addAction(projectDock_->toggleViewAction());
    view->addAction(propsDock_->toggleViewAction());
    view->addAction(timelineDock_->toggleViewAction());
    view->addAction(aResetLayout_);

    QMenu* help = menuBar()->addMenu("&Help");
    help->addAction(aAbout_);
}

void MainWindow::resetLayout() {
    restoreState(defaultState_);
    for (QDockWidget* d : {projectDock_, propsDock_, timelineDock_}) { d->setFloating(false); d->show(); }
}

void MainWindow::updateTitle() {
    setWindowTitle(QString("%1%2 - AFTER TRUE").arg(doc_->displayName(), doc_->isDirty() ? QStringLiteral("*") : QString()));
}

void MainWindow::updateActionStates() {
    const bool hasComp = doc_->activeComp() != nullptr;
    const bool hasSel = doc_->selectedLayer() != nullptr;
    for (QAction* a : {aCompSettings_, aNewSolid_, aPlay_, aExportFrame_}) a->setEnabled(hasComp);
    for (QAction* a : {aDuplicate_, aDelete_, aSplit_, aUp_, aDown_, aFit_, aRename_}) a->setEnabled(hasSel);
}

// ---------------- file actions ----------------
bool MainWindow::maybeSave() {
    if (!doc_->isDirty()) return true;
    const auto r = QMessageBox::question(this, "Unsaved changes",
        QString("Save changes to \"%1\" before continuing?").arg(doc_->displayName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (r == QMessageBox::Cancel) return false;
    if (r == QMessageBox::Save) return doSave();
    return true;
}

void MainWindow::doNewProject() {
    viewer_->stop();
    if (!maybeSave()) return;
    doc_->newProject();
}

void MainWindow::doOpen(const QString& pathIn) {
    viewer_->stop();
    if (!maybeSave()) return;
    QString path = pathIn;
    if (path.isEmpty())
        path = QFileDialog::getOpenFileName(this, "Open project", QString(), "AFTER TRUE projects (*.atrue)");
    if (path.isEmpty()) return;
    QString err;
    if (!doc_->openProject(path, err)) { QMessageBox::critical(this, "Cannot open project", err); return; }
    addRecent(path);
    timeline_->fitToComp();
    int missing = 0;
    for (const Footage& f : doc_->project().footage) if (!QFileInfo::exists(QString::fromStdString(f.path))) ++missing;
    if (missing > 0)
        QMessageBox::warning(this, "Missing media",
            QString("%1 media file(s) could not be found (moved or deleted).\nThey are marked in red in the Project panel.\nRight-click one > Relink to point it to the new location.").arg(missing));
}

bool MainWindow::doSave() {
    if (!doc_->hasFilePath()) return doSaveAs();
    QString err;
    if (!doc_->saveProject(err)) { QMessageBox::critical(this, "Save failed", err); return false; }
    QFile::remove(recoveryPath());
    return true;
}

bool MainWindow::doSaveAs() {
    QString path = QFileDialog::getSaveFileName(this, "Save project as", doc_->displayName(), "AFTER TRUE projects (*.atrue)");
    if (path.isEmpty()) return false;
    if (!path.endsWith(".atrue", Qt::CaseInsensitive)) path += ".atrue";
    QString err;
    if (!doc_->saveProjectAs(path, err)) { QMessageBox::critical(this, "Save failed", err); return false; }
    addRecent(path);
    QFile::remove(recoveryPath());
    return true;
}

void MainWindow::doImport() {
    const QStringList files = QFileDialog::getOpenFileNames(this, "Import footage", QString(),
        "Media files (*.mp4 *.mov *.mkv *.avi *.webm *.m4v *.png *.jpg *.jpeg *.webp *.gif *.bmp *.wav *.mp3 *.flac *.m4a *.ogg);;All files (*)");
    importPaths(files);
}

void MainWindow::importPaths(const QStringList& paths) {
    QStringList errors;
    for (const QString& p : paths) {
        QString err;
        if (!doc_->importFile(p, err)) errors << err;
    }
    if (!errors.isEmpty()) QMessageBox::warning(this, "Some files could not be imported", errors.join("\n\n"));
}

void MainWindow::doNewComp() {
    CompSettingsDialog dlg("New Composition", "Comp " + QString::number(doc_->project().comps.size() + 1), 1920, 1080, Rational{30, 1}, 10, this);
    if (dlg.exec() == QDialog::Accepted) doc_->addComposition(dlg.name(), dlg.compWidth(), dlg.compHeight(), dlg.fps(), dlg.duration());
}

void MainWindow::doCompSettings() {
    Composition* c = doc_->activeComp();
    if (!c) return;
    CompSettingsDialog dlg("Composition Settings", QString::fromStdString(c->name), c->width, c->height, c->fps, c->duration, this);
    if (dlg.exec() == QDialog::Accepted)
        doc_->updateComposition(c->id, dlg.name(), dlg.compWidth(), dlg.compHeight(), dlg.fps(), dlg.duration());
}

void MainWindow::doNewSolid() {
    const QColor c = QColorDialog::getColor(QColor(200, 40, 40), this, "Solid color");
    if (c.isValid()) doc_->addSolidLayer(c);
}

void MainWindow::doExportFrame() {
    QString path = QFileDialog::getSaveFileName(this, "Export current frame", "frame.png", "PNG image (*.png)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
    Frame f;
    viewer_->renderFull(f);
    if (f.empty()) { QMessageBox::warning(this, "Export", "Nothing to export."); return; }
    const QImage img(f.rgba.data(), f.width, f.height, f.width * 4, QImage::Format_RGBX8888);
    if (!img.save(path, "PNG")) QMessageBox::critical(this, "Export failed", "Could not write " + path);
    else statusBar()->showMessage("Saved " + path, 5000);
}

void MainWindow::doAbout() {
    QMessageBox::about(this, "AFTER TRUE",
        QString("<h3>AFTER TRUE %1 - Milestone 1</h3>"
                "<p><b>Working in this version:</b> importing video/images, multi-layer compositions, "
                "layer stacking, transforms (position/scale/rotation/anchor/opacity), keyframes with linear / easy-ease / hold "
                "interpolation, timeline editing (move, trim, split, solo, lock), saving and reopening projects, "
                "autosave and recovery, PNG frame export.</p>"
                "<p><b>NOT implemented yet:</b> effects, masks, blend modes, text and shape layers, graph editor, audio playback, "
                "video export (MP4/MOV), AI features, background removal, transitions, GPU acceleration, undo/redo.</p>"
                "<p>See docs/FEATURES.md for the full verification checklist.</p>").arg(QString::fromUtf8(AT_VERSION)));
}

// ---------------- recent files / recovery ----------------
void MainWindow::addRecent(const QString& path) {
    QSettings s("AfterTrue", "AfterTrue");
    QStringList l = s.value("recent").toStringList();
    l.removeAll(path); l.prepend(path);
    while (l.size() > 8) l.removeLast();
    s.setValue("recent", l);
    rebuildRecentMenu();
}

void MainWindow::rebuildRecentMenu() {
    recentMenu_->clear();
    const QStringList l = QSettings("AfterTrue", "AfterTrue").value("recent").toStringList();
    for (const QString& p : l) recentMenu_->addAction(p, this, [this, p] { doOpen(p); });
    recentMenu_->setEnabled(!l.isEmpty());
}

QString MainWindow::recoveryPath() const {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/autosave";
    QDir().mkpath(dir);
    return dir + "/recovery.atrue";
}

void MainWindow::autosave() {
    if (!doc_->isDirty()) return;
    QString err;
    doc_->writeRecoveryCopy(recoveryPath(), err);
}

void MainWindow::offerRecovery() {
    if (!QFileInfo::exists(recoveryPath())) return;
    const auto r = QMessageBox::question(this, "Recover unsaved work?",
        "AFTER TRUE did not close normally last time.\nDo you want to recover the autosaved project?");
    if (r == QMessageBox::Yes) {
        QString err;
        if (doc_->openRecoveryCopy(recoveryPath(), err)) { timeline_->fitToComp(); return; }
        QMessageBox::warning(this, "Recovery failed", err);
    }
    QFile::remove(recoveryPath());
}

void MainWindow::openPathOnStartup(const QString& path) { doOpen(path); }

// ---------------- window events ----------------
void MainWindow::closeEvent(QCloseEvent* e) {
    viewer_->stop();
    if (!maybeSave()) { e->ignore(); return; }
    QFile::remove(recoveryPath());
    QSettings s("AfterTrue", "AfterTrue");
    s.setValue("geometry", saveGeometry());
    s.setValue("windowState", saveState());
    e->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* e) {
    QStringList files;
    for (const QUrl& u : e->mimeData()->urls()) if (u.isLocalFile()) files << u.toLocalFile();
    if (files.size() == 1 && files.first().endsWith(".atrue", Qt::CaseInsensitive)) { doOpen(files.first()); return; }
    importPaths(files);
}
}
