#pragma once
#include <QMainWindow>
#include <QTimer>
class QAction; class QMenu; class QDockWidget; class QLabel;
namespace at {
class Document; class ViewerPanel; class ProjectPanel; class PropertiesPanel; class TimelineView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    void openPathOnStartup(const QString& path);
    void offerRecovery();
protected:
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
private:
    void buildActions();
    void buildMenus();
    void updateTitle();
    void updateActionStates();
    bool maybeSave();
    void doNewProject();
    void doOpen(const QString& path = QString());
    bool doSave();
    bool doSaveAs();
    void doImport();
    void importPaths(const QStringList& paths);
    void doNewComp();
    void doCompSettings();
    void doNewSolid();
    void doExportFrame();
    void doAbout();
    void addRecent(const QString& path);
    void rebuildRecentMenu();
    void autosave();
    QString recoveryPath() const;
    void resetLayout();

    Document* doc_;
    ViewerPanel* viewer_;
    ProjectPanel* projectPanel_;
    PropertiesPanel* props_;
    TimelineView* timeline_;
    QDockWidget *projectDock_, *propsDock_, *timelineDock_;
    QLabel* status_;
    QTimer autosaveTimer_;
    QByteArray defaultState_;

    QAction *aNew_, *aOpen_, *aSave_, *aSaveAs_, *aImport_, *aExportFrame_, *aQuit_;
    QAction *aNewComp_, *aCompSettings_;
    QAction *aNewSolid_, *aDuplicate_, *aDelete_, *aSplit_, *aUp_, *aDown_, *aFit_, *aRename_;
    QAction *aPlay_, *aStart_, *aEnd_, *aNext_, *aPrev_;
    QAction *aZoomIn_, *aZoomOut_, *aZoomFit_, *aResetLayout_, *aAbout_;
    QMenu* recentMenu_;
};
}
