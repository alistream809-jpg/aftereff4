#include "ui/CompSettingsDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>
#include <cmath>
namespace at {
namespace {
struct FpsPreset { const char* label; int num, den; };
const FpsPreset kFps[] = {{"23.976", 24000, 1001}, {"24", 24, 1}, {"25", 25, 1}, {"29.97", 30000, 1001},
                          {"30", 30, 1}, {"50", 50, 1}, {"59.94", 60000, 1001}, {"60", 60, 1}};
struct SizePreset { const char* label; int w, h; };
const SizePreset kSizes[] = {{"HD 720p (1280 x 720)", 1280, 720}, {"Full HD 1080p (1920 x 1080)", 1920, 1080},
                             {"QHD 1440p (2560 x 1440)", 2560, 1440}, {"4K UHD (3840 x 2160)", 3840, 2160},
                             {"Vertical 1080 x 1920", 1080, 1920}, {"Square 1080 x 1080", 1080, 1080}};
}
CompSettingsDialog::CompSettingsDialog(const QString& title, const QString& name, int w, int h, Rational fps, double duration, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(title);
    auto* form = new QFormLayout;
    name_ = new QLineEdit(name);
    preset_ = new QComboBox; preset_->addItem("Custom");
    for (const auto& s : kSizes) preset_->addItem(s.label);
    w_ = new QSpinBox; w_->setRange(16, 16384); w_->setValue(w);
    h_ = new QSpinBox; h_->setRange(16, 16384); h_->setValue(h);
    fps_ = new QComboBox;
    int sel = -1;
    for (int i = 0; i < 8; ++i) {
        fps_->addItem(QString("%1 fps").arg(QString::fromLatin1(kFps[i].label)), i);
        if (kFps[i].num == fps.num && kFps[i].den == fps.den) sel = i;
    }
    if (sel < 0) {   // custom rate from an existing project
        fps_->addItem(QString("%1 fps").arg(fps.toDouble(), 0, 'g', 6), 100);
        sel = fps_->count() - 1;
        fps_->setItemData(sel, fps.num, Qt::UserRole + 1);
        fps_->setItemData(sel, fps.den, Qt::UserRole + 2);
    }
    fps_->setCurrentIndex(sel);
    dur_ = new QDoubleSpinBox; dur_->setRange(0.04, 36000); dur_->setDecimals(2); dur_->setSuffix(" s"); dur_->setValue(duration);
    form->addRow("Name", name_);
    form->addRow("Preset", preset_);
    form->addRow("Width (px)", w_);
    form->addRow("Height (px)", h_);
    form->addRow("Frame rate", fps_);
    form->addRow("Duration", dur_);
    connect(preset_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        if (i > 0) { w_->setValue(kSizes[i - 1].w); h_->setValue(kSizes[i - 1].h); }
    });
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto* lay = new QVBoxLayout(this);
    lay->addLayout(form); lay->addWidget(bb);
}
QString CompSettingsDialog::name() const { return name_->text().trimmed().isEmpty() ? QStringLiteral("Comp") : name_->text().trimmed(); }
int CompSettingsDialog::compWidth() const { return w_->value(); }
int CompSettingsDialog::compHeight() const { return h_->value(); }
double CompSettingsDialog::duration() const { return dur_->value(); }
Rational CompSettingsDialog::fps() const {
    const int d = fps_->currentData().toInt();
    if (d >= 100) return Rational{fps_->currentData(Qt::UserRole + 1).toInt(), fps_->currentData(Qt::UserRole + 2).toInt()};
    return Rational{kFps[d].num, kFps[d].den};
}
}
