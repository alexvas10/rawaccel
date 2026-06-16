#include "Calculator.hpp"
#include <cmath>
#include <algorithm>

const double Calculator::SLOW_MOVEMENTS[] = {
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
    1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0,
    3.333, 3.666, 4.0, 4.333, 4.666
};

double Calculator::magnitude(double x, double y) {
    if (x == 0) return std::abs(y);
    if (y == 0) return std::abs(x);
    return std::sqrt(x*x + y*y);
}

std::vector<Calculator::SimInput> Calculator::make_sim_inputs(double max_velocity,
                                                               bool x_axis_only,
                                                               bool y_axis_only)
{
    std::vector<SimInput> inputs;

    double increment = max_velocity / RESOLUTION;

    for (double sv : SLOW_MOVEMENTS) {
        if (x_axis_only) {
            int cx = static_cast<int>(std::ceil(sv));
            double tf = cx / sv;
            inputs.push_back({sv, tf, cx, 0});
        } else if (y_axis_only) {
            int cy = static_cast<int>(std::ceil(sv));
            double tf = cy / sv;
            inputs.push_back({sv, tf, 0, cy});
        } else {
            // Combined: scale by 50 to get integer x values for slow speeds
            int cx = static_cast<int>(std::round(sv * 50.0));
            double cm = magnitude(cx, 0);
            double tf = cm / sv;
            inputs.push_back({sv, tf, cx, 0});
        }
    }

    for (double v = 5.0; v < max_velocity; v += increment) {
        if (y_axis_only) {
            int cy = static_cast<int>(std::ceil(v));
            double tf = cy / v;
            double vel = magnitude(0, cy) / tf;
            inputs.push_back({vel, tf, 0, cy});
        } else {
            int cx = static_cast<int>(std::ceil(v));
            double tf = cx / v;
            double vel = magnitude(cx, 0) / tf;
            inputs.push_back({vel, tf, cx, 0});
        }
    }

    std::sort(inputs.begin(), inputs.end(),
              [](const SimInput& a, const SimInput& b){ return a.velocity < b.velocity; });

    return inputs;
}

ChartData Calculator::compute(rawaccel::modifier_settings settings,
                               const std::vector<SimInput>& inputs)
{
    rawaccel::init_data(settings);
    rawaccel::modifier mod(settings);

    // Disable smoothers for charting (stateless)
    rawaccel::speed_args sa = settings.prof.speed_processor_args;
    sa.input_speed_smooth_halflife  = 0;
    sa.scale_smooth_halflife        = 0;
    sa.output_speed_smooth_halflife = 0;

    rawaccel::speed_processor sp;
    sp.init(sa);

    ChartData data;
    double last_in  = 0.0;
    double last_out = 0.0;
    bool first = true;

    for (const auto& inp : inputs) {
        if (inp.velocity <= 0) continue;

        vec2d v{static_cast<double>(inp.x), static_cast<double>(inp.y)};
        // dpi_factor = 1 for charting (matches Windows grapher behavior)
        mod.modify(v, sp, settings, 1.0, inp.time_ms);

        double out_mag = magnitude(v.x, v.y);
        double out_vel = out_mag / inp.time_ms;
        double ratio   = out_vel / inp.velocity;

        double gain_val = first ? ratio
                        : (inp.velocity - last_in > 1e-9)
                          ? (out_vel - last_out) / (inp.velocity - last_in)
                          : ratio;

        // Clamp to finite values
        if (!std::isfinite(ratio))   ratio    = 1.0;
        if (!std::isfinite(out_vel)) out_vel  = 0.0;
        if (!std::isfinite(gain_val)) gain_val = ratio;

        data.sensitivity.append({inp.velocity, ratio});
        data.velocity.append({inp.velocity, out_vel});
        data.gain.append({inp.velocity, gain_val});

        if (ratio > data.max_accel) data.max_accel = ratio;
        if (ratio < data.min_accel) data.min_accel = ratio;

        last_in  = inp.velocity;
        last_out = out_vel;
        first    = false;

        // Reinitialize speed processor between samples (stateless simulation)
        sp.init(sa);
    }

    return data;
}

AllChartData Calculator::calculate(const rawaccel::modifier_settings& settings, double dpi) {
    double max_v = dpi * MAX_MULTIPLIER;

    AllChartData all;
    all.combined = compute(settings, make_sim_inputs(max_v, false, false));
    all.x_axis   = compute(settings, make_sim_inputs(max_v, true,  false));
    all.y_axis   = compute(settings, make_sim_inputs(max_v, false, true));
    return all;
}

QPointF Calculator::calculate_dot(const rawaccel::modifier_settings& settings,
                                   int raw_x, int raw_y, double time_ms)
{
    if (time_ms <= 0) return {};
    rawaccel::modifier_settings s = settings;
    rawaccel::init_data(s);
    rawaccel::modifier mod(s);

    rawaccel::speed_args sa = s.prof.speed_processor_args;
    sa.input_speed_smooth_halflife  = 0;
    sa.scale_smooth_halflife        = 0;
    sa.output_speed_smooth_halflife = 0;

    rawaccel::speed_processor sp;
    sp.init(sa);

    vec2d v{static_cast<double>(raw_x), static_cast<double>(raw_y)};
    double in_vel = magnitude(v.x, v.y) / time_ms;

    mod.modify(v, sp, s, 1.0, time_ms);

    double out_vel = magnitude(v.x, v.y) / time_ms;
    double ratio   = in_vel > 0 ? out_vel / in_vel : 1.0;

    return {in_vel, ratio};
}
