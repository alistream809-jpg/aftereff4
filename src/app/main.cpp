#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include "ui/MainWindow.h"
#include "ui/Theme.h"

int main(int argc, char* argv[]) {
    QCoreApplication::setOrganizationName("AfterTrue");
    QCoreApplication::setApplicationName("AfterTrue");
    QApplication app(argc, argv);
    at::applyDarkTheme(app);

    at::MainWindow w;
    w.show();
    const QStringList args = QCoreApplication::arguments();
    if (args.size() > 1) {
        // started by double-clicking a .atrue file
        const QString path = args.at(1);
        QTimer::singleShot(0, &w, [&w, path] { w.openPathOnStartup(path); });
    } else {
        QTimer::singleShot(0, &w, [&w] { w.offerRecovery(); });
    }
    return app.exec();
}
