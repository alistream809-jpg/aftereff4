#pragma once
#include <QWidget>
#include <array>
#include "project/Project.h"
class QLabel; class QDoubleSpinBox; class QCheckBox; class QComboBox; class QPushButton;
namespace at {
class Document;
class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(Document& doc, QWidget* parent = nullptr);
private slots:
    void sync();
private:
    Document& doc_;
    QLabel* title_;
    QComboBox* interp_;
    QPushButton* colorBtn_;
    std::array<QDoubleSpinBox*, kPropCount> spins_{};
    std::array<QCheckBox*, kPropCount> keys_{};
    bool updating_ = false;
};
}
