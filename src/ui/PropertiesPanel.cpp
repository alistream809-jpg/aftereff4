#include "ui/PropertiesPanel.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include "ui/Document.h"

namespace at {

PropertiesPanel::PropertiesPanel(Document& doc, QWidget* parent) : QWidget(parent), doc_(doc) {
    auto* lay = new QVBoxLayout(this);
    title_ = new QLabel("No layer selected");
    title_->setWordWrap(true);
    QFont f = title_->font(); f.setBold(true); title_->setFont(f);
    lay->addWidget(title_);

    auto* grid = new QGridLayout;
    grid->addWidget(new QLabel("Property"), 0, 0);
    grid->addWidget(new QLabel("Value"), 0, 1);
    grid->addWidget(new QLabel("Animate"), 0, 2);
    for (int i = 0; i < kPropCount; ++i) {
        const Prop p = static_cast<Prop>(i);
        auto* sp = new QDoubleSpinBox;
        sp->setDecimals(2);
        sp->setRange(p == Prop::Opacity ? 0.0 : -100000.0, p == Prop::Opacity ? 100.0 : 100000.0);
        sp->setKeyboardTracking(false);
        sp->setMinimumWidth(100);
        auto* ck = new QCheckBox;
        ck->setToolTip("Turn on to record keyframes for this property (like the stopwatch in After Effects).\n"
                       "When on, changing the value at a different time creates a new keyframe.");
        spins_[static_cast<size_t>(i)] = sp; keys_[static_cast<size_t>(i)] = ck;
        grid->addWidget(new QLabel(propLabel(p)), i + 1, 0);
        grid->addWidget(sp, i + 1, 1);
        grid->addWidget(ck, i + 1, 2, Qt::AlignCenter);
        connect(sp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, p](double v) {
            if (updating_ || doc_.selectedLayerId().empty()) return;
            doc_.setPropertyValue(doc_.selectedLayerId(), p, v);
        });
        connect(ck, &QCheckBox::toggled, this, [this, p](bool on) {
            if (updating_ || doc_.selectedLayerId().empty()) return;
            doc_.setPropertyAnimated(doc_.selectedLayerId(), p, on);
        });
    }
    lay->addLayout(grid);

    auto* row = new QHBoxLayout;
    row->addWidget(new QLabel("New keyframes:"));
    interp_ = new QComboBox;
    interp_->addItem("Linear", static_cast<int>(Interp::Linear));
    interp_->addItem("Easy Ease", static_cast<int>(Interp::Bezier));
    interp_->addItem("Hold", static_cast<int>(Interp::Hold));
    row->addWidget(interp_, 1);
    lay->addLayout(row);
    connect(interp_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        doc_.setNewKeyInterp(static_cast<Interp>(interp_->currentData().toInt()));
    });

    colorBtn_ = new QPushButton("Solid color...");
    lay->addWidget(colorBtn_);
    connect(colorBtn_, &QPushButton::clicked, this, [this] {
        const Layer* l = doc_.selectedLayer();
        if (!l || l->type != LayerType::Solid) return;
        const QColor c = QColorDialog::getColor(QColor(l->solidR, l->solidG, l->solidB), this, "Solid color");
        if (c.isValid()) doc_.setSolidColor(doc_.selectedLayerId(), c);
    });
    lay->addStretch(1);

    connect(&doc_, &Document::changed, this, &PropertiesPanel::sync);
    connect(&doc_, &Document::timeChanged, this, [this](double) { sync(); });
    connect(&doc_, &Document::selectionChanged, this, &PropertiesPanel::sync);
    sync();
}

void PropertiesPanel::sync() {
    const Layer* l = doc_.selectedLayer();
    updating_ = true;
    colorBtn_->setVisible(l && l->type == LayerType::Solid);
    if (!l) {
        title_->setText("No layer selected");
        for (int i = 0; i < kPropCount; ++i) {
            spins_[static_cast<size_t>(i)]->setEnabled(false);
            keys_[static_cast<size_t>(i)]->setEnabled(false);
            keys_[static_cast<size_t>(i)]->setChecked(false);
        }
    } else {
        title_->setText("Layer: " + QString::fromUtf8(l->name.data(), static_cast<int>(l->name.size())));
        const double lt = doc_.time() - l->startTime;
        for (int i = 0; i < kPropCount; ++i) {
            const AnimatedProperty& ap = l->prop(static_cast<Prop>(i));
            auto* sp = spins_[static_cast<size_t>(i)];
            auto* ck = keys_[static_cast<size_t>(i)];
            sp->setEnabled(!l->locked);
            ck->setEnabled(!l->locked);
            const double v = ap.valueAt(lt);
            if (!qFuzzyCompare(sp->value() + 1.0, v + 1.0)) sp->setValue(v);
            ck->setChecked(ap.isAnimated());
        }
    }
    updating_ = false;
}
}
