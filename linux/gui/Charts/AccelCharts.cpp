#include "Charts/AccelCharts.hpp"
#include <QVBoxLayout>

AccelCharts::AccelCharts(QWidget* parent) : QWidget(parent) {
    auto* vl = new QVBoxLayout(this);
    vl->setContentsMargins(0, 0, 0, 0);
    vl->setSpacing(4);

    splitter_ = new QSplitter(Qt::Vertical, this);

    sensitivity_ = new ChartXY("Sensitivity", this);
    velocity_    = new ChartXY("Velocity",    this);
    gain_        = new ChartXY("Gain",        this);

    splitter_->addWidget(sensitivity_);
    splitter_->addWidget(velocity_);
    splitter_->addWidget(gain_);

    // Sensitivity gets more space by default
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    splitter_->setStretchFactor(2, 2);

    vl->addWidget(splitter_);

    setShowVelocityAndGain(false);
}

void AccelCharts::setShowVelocityAndGain(bool show) {
    velocity_->setVisible(show);
    gain_->setVisible(show);
}

void AccelCharts::setCombined(bool combined) {
    combined_ = combined;
    sensitivity_->setMode(combined);
    velocity_->setMode(combined);
    gain_->setMode(combined);
}

void AccelCharts::calculate(const rawaccel::modifier_settings& settings, double dpi) {
    AllChartData data = calc_.calculate(settings, dpi);

    if (combined_) {
        sensitivity_->setCombinedData(data.combined.sensitivity);
        velocity_->setCombinedData(data.combined.velocity);
        gain_->setCombinedData(data.combined.gain);
    } else {
        sensitivity_->setXData(data.x_axis.sensitivity);
        sensitivity_->setYData(data.y_axis.sensitivity);
        velocity_->setXData(data.x_axis.velocity);
        velocity_->setYData(data.y_axis.velocity);
        gain_->setXData(data.x_axis.gain);
        gain_->setYData(data.y_axis.gain);
    }
}

void AccelCharts::addLiveDot(const QPointF& pt) {
    sensitivity_->setDot(pt);
}

void AccelCharts::clearDots() {
    sensitivity_->clearDot();
    velocity_->clearDot();
    gain_->clearDot();
}
