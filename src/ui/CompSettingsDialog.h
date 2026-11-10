#pragma once
#include <QDialog>
#include "core/Rational.h"
class QLineEdit; class QSpinBox; class QDoubleSpinBox; class QComboBox;
namespace at {
class CompSettingsDialog : public QDialog {
    Q_OBJECT
public:
    CompSettingsDialog(const QString& title, const QString& name, int w, int h, Rational fps, double duration, QWidget* parent = nullptr);
    QString name() const;
    int compWidth() const;
    int compHeight() const;
    Rational fps() const;
    double duration() const;
private:
    QLineEdit* name_; QSpinBox* w_; QSpinBox* h_; QComboBox* preset_; QComboBox* fps_; QDoubleSpinBox* dur_;
};
}
