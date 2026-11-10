#include "ui/Theme.h"
#include <QApplication>
#include <QPalette>
#include <QStyleFactory>
namespace at {
void applyDarkTheme(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    const QColor bg(36, 36, 40), base(27, 27, 30), text(220, 220, 225), accent(60, 130, 220);
    p.setColor(QPalette::Window, bg);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, bg);
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, QColor(52, 52, 58));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 125));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 125));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 125));
    app.setPalette(p);
    app.setStyleSheet(
        "QDockWidget { font-weight: bold; }"
        "QDockWidget::title { background: #2b2b30; padding: 5px; }"
        "QToolTip { color: #ddd; background: #1b1b1e; border: 1px solid #555; }"
        "QMenu::separator { height: 1px; background: #444; margin: 3px 6px; }");
}
}
