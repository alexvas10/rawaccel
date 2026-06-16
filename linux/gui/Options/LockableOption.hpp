#pragma once

#include "Options/OptionWidget.hpp"
#include <QPushButton>

// OptionWidget with a lock button — when locked, emits lockedValueChanged
// which the parent uses to synchronize another control (e.g. Y follows X).
class LockableOption : public QWidget {
    Q_OBJECT
public:
    explicit LockableOption(const QString& label, QWidget* parent = nullptr);

    void setValue(double v);
    double value() const;
    void setActiveValue(double v);
    bool isLocked() const;

signals:
    void valueChanged(double v);
    void lockedValueChanged(double v); // emitted only when locked

private:
    OptionWidget* opt_;
    QPushButton*  lock_btn_;
    bool locked_ = false;
};
