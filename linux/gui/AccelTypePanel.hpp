#pragma once

#include "shared/compat.hpp"
#include "rawaccel-base.hpp"
#include "Layouts/LayoutBase.hpp"
#include "Options/OptionWidget.hpp"
#include "Options/LutEditor.hpp"

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>

// One acceleration axis panel (X or Y).
// Equivalent to C# AccelTypeOptions + per-mode Layouts.
class AccelTypePanel : public QWidget {
    Q_OBJECT
public:
    explicit AccelTypePanel(const QString& title = "Acceleration", QWidget* parent = nullptr);

    void fillArgs(rawaccel::accel_args& args) const;
    void setArgs(const rawaccel::accel_args& args);

    // Copy X layout/values to Y (used when "same for X and Y" is checked).
    void copyFrom(const AccelTypePanel& other);

    bool isLogarithmic() const;

signals:
    void changed();

private slots:
    void onModeChanged(int index);

private:
    QGroupBox*   group_;
    QComboBox*   mode_combo_;
    QCheckBox*   gain_switch_;

    // Classic / Linear caps
    QComboBox*    cap_type_classic_;
    OptionWidget* in_cap_classic_;
    OptionWidget* out_cap_classic_;

    // Power caps
    QComboBox*    cap_type_power_;
    OptionWidget* in_cap_power_;
    OptionWidget* out_cap_power_;

    // Jump caps
    OptionWidget* in_jump_;
    OptionWidget* out_jump_;

    OptionWidget* input_offset_;
    OptionWidget* output_offset_;
    OptionWidget* accel_;
    OptionWidget* scale_;
    OptionWidget* exponent_classic_;
    OptionWidget* exponent_power_;
    OptionWidget* decay_rate_;
    OptionWidget* gamma_;
    OptionWidget* motivity_;
    OptionWidget* smooth_;
    OptionWidget* limit_;
    OptionWidget* sync_speed_;

    LutEditor*   lut_editor_;

    const ModeLayout* current_layout_ = &Layouts::Off;

    void apply_layout(const ModeLayout* layout);
    void emit_changed();

    // Helper: show/hide a widget
    static void show_if(QWidget* w, bool show) { w->setVisible(show); }
};
