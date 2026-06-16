#pragma once

#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QHBoxLayout>

// Labeled numeric input + active value label.
// Equivalent to C# Option / OptionBase.
class OptionWidget : public QWidget {
    Q_OBJECT
public:
    explicit OptionWidget(const QString& label, QWidget* parent = nullptr);

    void setLabel(const QString& text);
    void setValue(double v);
    double value() const;
    void setActiveValue(double v, const QString& fmt = "0.######");
    void setRange(double lo, double hi);
    void setDecimals(int d);
    void setSingleStep(double s);

signals:
    void valueChanged(double v);

private:
    QLabel*       label_;
    QDoubleSpinBox* spin_;
    QLabel*       active_label_;
};
