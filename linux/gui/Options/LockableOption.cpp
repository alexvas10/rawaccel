#include "Options/LockableOption.hpp"
#include <QHBoxLayout>

LockableOption::LockableOption(const QString& label, QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    opt_ = new OptionWidget(label, this);
    lock_btn_ = new QPushButton("🔓", this);
    lock_btn_->setFixedSize(24, 24);
    lock_btn_->setCheckable(true);
    lock_btn_->setToolTip("Lock Y to X");

    layout->addWidget(opt_);
    layout->addWidget(lock_btn_);

    connect(opt_, &OptionWidget::valueChanged, this, [this](double v) {
        emit valueChanged(v);
        if (locked_) emit lockedValueChanged(v);
    });

    connect(lock_btn_, &QPushButton::toggled, this, [this](bool on) {
        locked_ = on;
        lock_btn_->setText(on ? "🔒" : "🔓");
    });
}

void LockableOption::setValue(double v) { opt_->setValue(v); }
double LockableOption::value() const { return opt_->value(); }
void LockableOption::setActiveValue(double v) { opt_->setActiveValue(v); }
bool LockableOption::isLocked() const { return locked_; }
