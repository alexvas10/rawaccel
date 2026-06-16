#include "Options/OptionWidget.hpp"

OptionWidget::OptionWidget(const QString& label, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    label_ = new QLabel(label, this);
    label_->setMinimumWidth(130);

    spin_ = new QDoubleSpinBox(this);
    spin_->setDecimals(6);
    spin_->setRange(-1e9, 1e9);
    spin_->setSingleStep(0.1);
    spin_->setMinimumWidth(100);

    active_label_ = new QLabel("—", this);
    active_label_->setMinimumWidth(90);
    active_label_->setStyleSheet("color: gray;");

    layout->addWidget(label_);
    layout->addWidget(spin_);
    layout->addWidget(active_label_);
    layout->addStretch();

    connect(spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &OptionWidget::valueChanged);
}

void OptionWidget::setLabel(const QString& text) { label_->setText(text); }
void OptionWidget::setValue(double v) {
    QSignalBlocker blk(spin_);
    spin_->setValue(v);
}
double OptionWidget::value() const { return spin_->value(); }

void OptionWidget::setActiveValue(double v, const QString& /*fmt*/) {
    active_label_->setText(QString::number(v, 'g', 6));
}

void OptionWidget::setRange(double lo, double hi) { spin_->setRange(lo, hi); }
void OptionWidget::setDecimals(int d) { spin_->setDecimals(d); }
void OptionWidget::setSingleStep(double s) { spin_->setSingleStep(s); }
