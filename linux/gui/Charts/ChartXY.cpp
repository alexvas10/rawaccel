#include "Charts/ChartXY.hpp"
#include <QHBoxLayout>
#include <algorithm>

ChartXY::ChartXY(const QString& title, QWidget* parent) : QWidget(parent) {
    auto* hl = new QHBoxLayout(this);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(4);

    chart_combined_ = make_chart(title);
    chart_x_        = make_chart(title + " X");
    chart_y_        = make_chart(title + " Y");

    series_combined_       = new QLineSeries;
    series_x_on_combined_  = new QLineSeries;
    series_x_              = new QLineSeries;
    series_y_              = new QLineSeries;
    dot_combined_          = new QScatterSeries;
    dot_x_                 = new QScatterSeries;

    series_combined_->setName("Combined");
    series_x_on_combined_->setName("X");
    series_x_->setName("X");
    series_y_->setName("Y");
    dot_combined_->setName("Last");
    dot_x_->setName("Last");

    series_x_on_combined_->setColor(Qt::blue);
    series_x_->setColor(Qt::blue);
    series_y_->setColor(Qt::red);
    dot_combined_->setMarkerSize(8);
    dot_x_->setMarkerSize(8);
    dot_combined_->setColor(Qt::darkGreen);
    dot_x_->setColor(Qt::darkGreen);

    QPen pen; pen.setWidth(2);
    series_combined_->setPen(pen);
    series_x_on_combined_->setPen(pen);
    series_x_->setPen(pen);
    series_y_->setPen(pen);

    chart_combined_->addSeries(series_combined_);
    chart_combined_->addSeries(series_x_on_combined_);
    chart_combined_->addSeries(dot_combined_);
    chart_x_->addSeries(series_x_);
    chart_x_->addSeries(dot_x_);
    chart_y_->addSeries(series_y_);

    chart_combined_->createDefaultAxes();
    chart_x_->createDefaultAxes();
    chart_y_->createDefaultAxes();

    view_combined_ = make_view(chart_combined_);
    view_x_        = make_view(chart_x_);
    view_y_        = make_view(chart_y_);

    hl->addWidget(view_combined_);
    hl->addWidget(view_x_);
    hl->addWidget(view_y_);

    setMode(true);
}

QChart* ChartXY::make_chart(const QString& title) {
    auto* c = new QChart;
    c->setTitle(title);
    c->legend()->setVisible(false);
    c->setMargins(QMargins(4, 4, 4, 4));
    return c;
}

QChartView* ChartXY::make_view(QChart* chart) {
    auto* v = new QChartView(chart);
    v->setRenderHint(QPainter::Antialiasing);
    v->setRubberBand(QChartView::RectangleRubberBand);
    v->setMinimumHeight(200);
    return v;
}

void ChartXY::setMode(bool combined) {
    is_combined_ = combined;
    view_combined_->setVisible(combined);
    view_x_->setVisible(!combined);
    view_y_->setVisible(!combined);
}

void ChartXY::setCombinedData(const QList<QPointF>& pts) {
    series_combined_->replace(pts);
    update_axes(chart_combined_, pts);
}

void ChartXY::setXData(const QList<QPointF>& pts) {
    series_x_on_combined_->replace(pts);
    series_x_->replace(pts);
    update_axes(chart_x_, pts);
}

void ChartXY::setYData(const QList<QPointF>& pts) {
    series_y_->replace(pts);
    update_axes(chart_y_, pts);
}

void ChartXY::setDot(const QPointF& pt) {
    dot_combined_->clear();
    dot_x_->clear();
    dot_combined_->append(pt);
    dot_x_->append(pt);
}

void ChartXY::clearDot() {
    dot_combined_->clear();
    dot_x_->clear();
}

void ChartXY::setLogScaleX(bool /*log_x*/) {
    // Qt6Charts supports QLogValueAxis — skip for now, use linear
}

void ChartXY::update_axes(QChart* chart, const QList<QPointF>& pts) {
    if (pts.isEmpty()) return;
    double min_x = pts.first().x(), max_x = pts.last().x();
    double min_y = 1e9, max_y = -1e9;
    for (const auto& p : pts) {
        min_y = std::min(min_y, p.y());
        max_y = std::max(max_y, p.y());
    }
    double pad_y = (max_y - min_y) * 0.05;
    if (pad_y == 0) pad_y = 0.1;

    auto axes = chart->axes();
    if (axes.size() >= 2) {
        if (auto* ax = qobject_cast<QValueAxis*>(axes[0])) ax->setRange(min_x, max_x);
        if (auto* ay = qobject_cast<QValueAxis*>(axes[1])) ay->setRange(min_y - pad_y, max_y + pad_y);
    }
}
