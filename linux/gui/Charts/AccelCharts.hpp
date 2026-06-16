#pragma once

#include "Charts/ChartXY.hpp"
#include "Calculator.hpp"

#include <QWidget>
#include <QSplitter>
#include <QCheckBox>

// Manages the three chart rows: Sensitivity, Velocity, Gain.
// Equivalent to C# AccelCharts.
class AccelCharts : public QWidget {
    Q_OBJECT
public:
    explicit AccelCharts(QWidget* parent = nullptr);

    // Recompute all chart data from settings.
    void calculate(const rawaccel::modifier_settings& settings, double dpi);

    // Show or hide the velocity and gain rows.
    void setShowVelocityAndGain(bool show);

    // Show combined (whole) or separate X/Y charts.
    void setCombined(bool combined);

    // Add a live mouse movement dot overlay.
    void addLiveDot(const QPointF& sensitivity_pt);
    void clearDots();

private:
    ChartXY* sensitivity_;
    ChartXY* velocity_;
    ChartXY* gain_;
    QSplitter* splitter_;
    Calculator calc_;
    bool combined_ = true;
};
