#pragma once

#include "shared/compat.hpp"
#include "rawaccel-base.hpp"
#include <QString>

// Data-driven layout descriptor — which controls to show per acceleration mode.
// Replaces the C# inheritance chain (ClassicLayout, PowerLayout, etc.).
struct ModeLayout {
    QString display_name;
    rawaccel::accel_mode mode;

    bool show_gain_switch    = false;
    bool show_input_offset   = false;
    bool show_output_offset  = false;
    bool show_accel          = false; // "acceleration" field
    bool show_scale          = false; // "scale" field (power mode)
    bool show_power_classic  = false; // exponentClassic
    bool show_exponent_power = false; // exponentPower
    bool show_decay_rate     = false;
    bool show_gamma          = false;
    bool show_motivity       = false;
    bool show_smooth         = false;
    bool show_limit          = false;
    bool show_sync_speed     = false;
    bool show_cap_classic    = false; // cap type + in/out caps for classic/linear
    bool show_cap_power      = false; // cap type + caps for power
    bool show_jump_caps      = false; // input/output jump caps
    bool show_lut_editor     = false;
    bool logarithmic_charts  = false;
};

// Static layout definitions — one per acceleration mode shown in the dropdown.
namespace Layouts {

inline const ModeLayout Off{
    "Off", rawaccel::accel_mode::noaccel
};

inline const ModeLayout Classic{
    "Classic", rawaccel::accel_mode::classic,
    .show_gain_switch   = true,
    .show_input_offset  = true,
    .show_accel         = true,
    .show_power_classic = true,
    .show_cap_classic   = true,
};

inline const ModeLayout Linear{
    "Linear", rawaccel::accel_mode::classic, // same C++ mode, linear is classic with exp=1
    .show_gain_switch   = true,
    .show_input_offset  = true,
    .show_accel         = true,
    .show_cap_classic   = true,
};

inline const ModeLayout Natural{
    "Natural", rawaccel::accel_mode::natural,
    .show_gain_switch  = true,
    .show_input_offset = true,
    .show_decay_rate   = true,
    .show_limit        = true,
};

inline const ModeLayout Synchronous{
    "Synchronous", rawaccel::accel_mode::synchronous,
    .show_gain_switch   = true,
    .show_gamma         = true,
    .show_motivity      = true,
    .show_smooth        = true,
    .show_sync_speed    = true,
    .logarithmic_charts = true,
};

inline const ModeLayout Power{
    "Power", rawaccel::accel_mode::power,
    .show_gain_switch    = true,
    .show_output_offset  = true,
    .show_scale          = true,
    .show_exponent_power = true,
    .show_cap_power      = true,
};

inline const ModeLayout Jump{
    "Jump", rawaccel::accel_mode::jump,
    .show_gain_switch = true,
    .show_smooth      = true,
    .show_jump_caps   = true,
};

inline const ModeLayout LUT{
    "LUT", rawaccel::accel_mode::lookup,
    .show_gain_switch = true,
    .show_lut_editor  = true,
};

// Ordered list for the dropdown (index → layout)
inline const ModeLayout* ALL[] = {
    &Off, &Classic, &Linear, &Natural, &Synchronous, &Power, &Jump, &LUT
};
inline constexpr int COUNT = 8;

} // namespace Layouts
