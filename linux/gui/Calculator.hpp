#pragma once

#include "shared/compat.hpp"
#include "rawaccel.hpp"

#include <QPointF>
#include <QList>
#include <vector>

struct ChartData {
    QList<QPointF> sensitivity; // (in_velocity, sensitivity_ratio)
    QList<QPointF> velocity;    // (in_velocity, out_velocity)
    QList<QPointF> gain;        // (in_velocity, gain)
    double max_accel = 1.0;
    double min_accel = 1.0;
};

// Chart data sets: combined and separate X/Y
struct AllChartData {
    ChartData combined;
    ChartData x_axis;
    ChartData y_axis;
};

class Calculator {
public:
    Calculator() = default;

    // dpi affects the max velocity range on the x-axis.
    // Returns chart data for the given profile settings.
    AllChartData calculate(const rawaccel::modifier_settings& settings, double dpi = 1600.0);

    // Single dot for a specific real mouse event (for overlay).
    // Returns {in_velocity, out_sensitivity_ratio}.
    QPointF calculate_dot(const rawaccel::modifier_settings& settings,
                          int raw_x, int raw_y, double time_ms);

private:
    struct SimInput {
        double velocity;
        double time_ms;
        int x;
        int y;
    };

    static constexpr double MAX_MULTIPLIER = 0.05;
    static constexpr int    RESOLUTION     = 500;
    static constexpr int    ANGLE_DIVISIONS = 19;

    static const double SLOW_MOVEMENTS[];

    std::vector<SimInput> make_sim_inputs(double max_velocity, bool x_axis_only, bool y_axis_only);
    ChartData compute(rawaccel::modifier_settings settings,
                      const std::vector<SimInput>& inputs);

    static double magnitude(double x, double y);
};
