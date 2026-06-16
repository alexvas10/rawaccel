#pragma once

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

// One chart panel that can show a single combined line or X+Y lines.
// Equivalent to C# ChartXY.
class ChartXY : public QWidget {
    Q_OBJECT
public:
    explicit ChartXY(const QString& title, QWidget* parent = nullptr);

    // Set combined (single) series data
    void setCombinedData(const QList<QPointF>& pts);

    // Set separate X and Y series data
    void setXData(const QList<QPointF>& pts);
    void setYData(const QList<QPointF>& pts);

    // Show as one chart (combined) or two (X and Y side by side)
    void setMode(bool combined);

    // Add/clear live mouse movement dot
    void setDot(const QPointF& pt);
    void clearDot();

    void setLogScaleX(bool log_x);

private:
    QChartView* view_combined_;
    QChartView* view_x_;
    QChartView* view_y_;

    QChart* chart_combined_;
    QChart* chart_x_;
    QChart* chart_y_;

    QLineSeries*    series_combined_;
    QLineSeries*    series_x_on_combined_; // Y overlaid on combined chart
    QLineSeries*    series_x_;
    QLineSeries*    series_y_;
    QScatterSeries* dot_combined_;
    QScatterSeries* dot_x_;

    bool is_combined_ = true;

    static QChart* make_chart(const QString& title);
    static QChartView* make_view(QChart* chart);
    void update_axes(QChart* chart, const QList<QPointF>& pts);
};
