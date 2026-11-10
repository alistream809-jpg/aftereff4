#pragma once
#include <QMap>
#include <QWidget>
#include <QIcon>
class QTreeWidget; class QTreeWidgetItem;
namespace at {
class Document;
class ProjectPanel : public QWidget {
    Q_OBJECT
public:
    explicit ProjectPanel(Document& doc, QWidget* parent = nullptr);
signals:
    void importRequested();
private slots:
    void rebuild();
    void onDoubleClicked(QTreeWidgetItem* item);
    void onContextMenu(const QPoint& pos);
private:
    QIcon thumbnailFor(const std::string& footageId, const std::string& path);
    Document& doc_;
    QTreeWidget* tree_;
    QMap<QString, QIcon> thumbs_;   // keyed by footage id + path
};
}
