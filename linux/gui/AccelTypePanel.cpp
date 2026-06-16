#include "AccelTypePanel.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

AccelTypePanel::AccelTypePanel(const QString& title, QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    group_ = new QGroupBox(title, this);
    auto* vl = new QVBoxLayout(group_);
    outer->addWidget(group_);

    // Mode selector
    mode_combo_ = new QComboBox(this);
    for (int i = 0; i < Layouts::COUNT; ++i)
        mode_combo_->addItem(Layouts::ALL[i]->display_name);
    vl->addWidget(mode_combo_);

    // Gain / Legacy switch
    gain_switch_ = new QCheckBox("Gain / Velocity", this);
    gain_switch_->setChecked(true);
    vl->addWidget(gain_switch_);

    // Classic/Linear caps
    auto add_cap_row = [&](const QString& label) -> std::pair<QComboBox*, OptionWidget*> {
        auto* hl = new QHBoxLayout;
        auto* lbl = new QLabel(label, this);
        lbl->setMinimumWidth(60);
        auto* combo = new QComboBox(this);
        combo->addItems({"Output Cap", "Input Cap", "Both (In/Out)"});
        hl->addWidget(lbl);
        hl->addWidget(combo);
        vl->addLayout(hl);
        auto* opt = new OptionWidget("Cap", this);
        return {combo, opt};
    };

    {
        auto [c, o] = add_cap_row("Cap type");
        cap_type_classic_ = c;
        out_cap_classic_  = o;
        in_cap_classic_   = new OptionWidget("Input Cap", this);
        vl->addWidget(in_cap_classic_);
        vl->addWidget(out_cap_classic_);
    }
    {
        auto [c, o] = add_cap_row("Cap type (power)");
        cap_type_power_ = c;
        out_cap_power_  = o;
        in_cap_power_   = new OptionWidget("Input Cap", this);
        vl->addWidget(in_cap_power_);
        vl->addWidget(out_cap_power_);
    }

    // Jump caps
    in_jump_  = new OptionWidget("Input Jump",  this);
    out_jump_ = new OptionWidget("Output Jump", this);
    vl->addWidget(in_jump_);
    vl->addWidget(out_jump_);

    // Numeric options
    input_offset_    = new OptionWidget("Input Offset",   this);
    output_offset_   = new OptionWidget("Output Offset",  this);
    accel_           = new OptionWidget("Acceleration",   this);
    scale_           = new OptionWidget("Scale",          this);
    exponent_classic_= new OptionWidget("Exponent",       this);
    exponent_power_  = new OptionWidget("Exponent",       this);
    decay_rate_      = new OptionWidget("Decay Rate",     this);
    gamma_           = new OptionWidget("Gamma",          this);
    motivity_        = new OptionWidget("Motivity",       this);
    smooth_          = new OptionWidget("Smooth",         this);
    limit_           = new OptionWidget("Limit",          this);
    sync_speed_      = new OptionWidget("Sync Speed",     this);

    for (auto* w : {input_offset_, output_offset_, accel_, scale_,
                    exponent_classic_, exponent_power_, decay_rate_,
                    gamma_, motivity_, smooth_, limit_, sync_speed_})
        vl->addWidget(w);

    lut_editor_ = new LutEditor(this);
    vl->addWidget(lut_editor_);

    // Set reasonable defaults
    accel_->setValue(0.005);
    accel_->setRange(0, 1e6);
    scale_->setValue(1.0);
    exponent_classic_->setValue(2.0);
    exponent_classic_->setRange(1.001, 1e6);
    exponent_power_->setValue(0.05);
    exponent_power_->setRange(0.001, 1e6);
    decay_rate_->setValue(0.1);
    gamma_->setValue(1.0);
    motivity_->setValue(1.5);
    motivity_->setRange(1.001, 1e6);
    smooth_->setValue(0.5);
    smooth_->setRange(0.0, 1.0);
    limit_->setValue(1.5);
    limit_->setRange(1.001, 1e6);
    sync_speed_->setValue(5.0);
    in_cap_classic_->setValue(15.0);
    out_cap_classic_->setValue(1.5);
    in_cap_power_->setValue(15.0);
    out_cap_power_->setValue(1.5);
    in_jump_->setValue(5.0);
    out_jump_->setValue(1.5);

    // Signals
    auto connect_changed = [this](QWidget* w) {
        if (auto* opt = qobject_cast<OptionWidget*>(w))
            connect(opt, &OptionWidget::valueChanged, this, &AccelTypePanel::emit_changed);
    };
    for (auto* w : {input_offset_, output_offset_, accel_, scale_,
                    exponent_classic_, exponent_power_, decay_rate_,
                    gamma_, motivity_, smooth_, limit_, sync_speed_,
                    in_cap_classic_, out_cap_classic_,
                    in_cap_power_, out_cap_power_,
                    in_jump_, out_jump_})
        connect_changed(w);

    connect(gain_switch_, &QCheckBox::toggled, this, &AccelTypePanel::emit_changed);
    connect(cap_type_classic_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AccelTypePanel::emit_changed);
    connect(cap_type_power_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AccelTypePanel::emit_changed);
    connect(lut_editor_, &LutEditor::pointsChanged, this, &AccelTypePanel::emit_changed);
    connect(mode_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AccelTypePanel::onModeChanged);

    apply_layout(&Layouts::Off);
}

void AccelTypePanel::onModeChanged(int index) {
    if (index < 0 || index >= Layouts::COUNT) return;
    apply_layout(Layouts::ALL[index]);
    emit_changed();
}

void AccelTypePanel::apply_layout(const ModeLayout* layout) {
    current_layout_ = layout;
    bool off = (layout == &Layouts::Off);

    gain_switch_->setVisible(!off && layout->show_gain_switch);
    input_offset_->setVisible(layout->show_input_offset);
    output_offset_->setVisible(layout->show_output_offset);
    accel_->setVisible(layout->show_accel);
    scale_->setVisible(layout->show_scale);
    exponent_classic_->setVisible(layout->show_power_classic);
    exponent_power_->setVisible(layout->show_exponent_power);
    decay_rate_->setVisible(layout->show_decay_rate);
    gamma_->setVisible(layout->show_gamma);
    motivity_->setVisible(layout->show_motivity);
    smooth_->setVisible(layout->show_smooth);
    limit_->setVisible(layout->show_limit);
    sync_speed_->setVisible(layout->show_sync_speed);

    cap_type_classic_->setVisible(layout->show_cap_classic);
    in_cap_classic_->setVisible(layout->show_cap_classic);
    out_cap_classic_->setVisible(layout->show_cap_classic);

    cap_type_power_->setVisible(layout->show_cap_power);
    in_cap_power_->setVisible(layout->show_cap_power);
    out_cap_power_->setVisible(layout->show_cap_power);

    in_jump_->setVisible(layout->show_jump_caps);
    out_jump_->setVisible(layout->show_jump_caps);

    lut_editor_->setVisible(layout->show_lut_editor);
}

void AccelTypePanel::emit_changed() { emit changed(); }

bool AccelTypePanel::isLogarithmic() const {
    return current_layout_ ? current_layout_->logarithmic_charts : false;
}

void AccelTypePanel::fillArgs(rawaccel::accel_args& a) const {
    a.mode            = current_layout_->mode;
    a.gain            = gain_switch_->isChecked();
    a.input_offset    = input_offset_->value();
    a.output_offset   = output_offset_->value();
    a.acceleration    = accel_->value();
    a.scale           = scale_->value();
    a.exponent_classic= exponent_classic_->value();
    a.exponent_power  = exponent_power_->value();
    a.decay_rate      = decay_rate_->value();
    a.gamma           = gamma_->value();
    a.motivity        = motivity_->value();
    a.smooth          = smooth_->value();
    a.limit           = limit_->value();
    a.sync_speed      = sync_speed_->value();

    // Cap mode and values
    if (current_layout_->show_cap_classic) {
        int ci = cap_type_classic_->currentIndex();
        a.cap_mode = (ci == 0) ? rawaccel::cap_mode::out
                   : (ci == 1) ? rawaccel::cap_mode::in
                               : rawaccel::cap_mode::io;
        a.cap.x = in_cap_classic_->value();
        a.cap.y = out_cap_classic_->value();
    } else if (current_layout_->show_cap_power) {
        int ci = cap_type_power_->currentIndex();
        a.cap_mode = (ci == 0) ? rawaccel::cap_mode::out
                   : (ci == 1) ? rawaccel::cap_mode::in
                               : rawaccel::cap_mode::io;
        a.cap.x = in_cap_power_->value();
        a.cap.y = out_cap_power_->value();
    } else if (current_layout_->show_jump_caps) {
        a.cap.x = in_jump_->value();
        a.cap.y = out_jump_->value();
    }

    // LUT
    if (current_layout_->show_lut_editor) {
        auto pts = lut_editor_->points();
        a.length = 0;
        for (size_t i = 0; i < pts.size() && a.length + 1 < (int)rawaccel::LUT_RAW_DATA_CAPACITY; ++i) {
            a.data[a.length++] = pts[i].first;
            a.data[a.length++] = pts[i].second;
        }
    }

    // Linear mode: exponentClassic = 1 (forced)
    if (current_layout_ == &Layouts::Linear)
        a.exponent_classic = 1.0;
}

void AccelTypePanel::setArgs(const rawaccel::accel_args& a) {
    // Find matching layout
    const ModeLayout* layout = &Layouts::Off;
    for (int i = 0; i < Layouts::COUNT; ++i) {
        if (Layouts::ALL[i]->mode == a.mode) {
            // Special: Linear vs Classic distinguished by exponent
            if (Layouts::ALL[i] == &Layouts::Classic && a.exponent_classic == 1.0)
                continue;
            layout = Layouts::ALL[i];
            break;
        }
    }

    {
        QSignalBlocker blk(mode_combo_);
        for (int i = 0; i < Layouts::COUNT; ++i)
            if (Layouts::ALL[i] == layout) { mode_combo_->setCurrentIndex(i); break; }
    }
    apply_layout(layout);

    gain_switch_->setChecked(a.gain);
    input_offset_->setValue(a.input_offset);
    output_offset_->setValue(a.output_offset);
    accel_->setValue(a.acceleration);
    scale_->setValue(a.scale);
    exponent_classic_->setValue(a.exponent_classic);
    exponent_power_->setValue(a.exponent_power);
    decay_rate_->setValue(a.decay_rate);
    gamma_->setValue(a.gamma);
    motivity_->setValue(a.motivity);
    smooth_->setValue(a.smooth);
    limit_->setValue(a.limit);
    sync_speed_->setValue(a.sync_speed);

    // Caps
    auto set_cap_mode = [](QComboBox* cb, rawaccel::cap_mode m) {
        cb->setCurrentIndex(m == rawaccel::cap_mode::out ? 0
                          : m == rawaccel::cap_mode::in  ? 1 : 2);
    };
    set_cap_mode(cap_type_classic_, a.cap_mode);
    set_cap_mode(cap_type_power_, a.cap_mode);
    in_cap_classic_->setValue(a.cap.x);
    out_cap_classic_->setValue(a.cap.y);
    in_cap_power_->setValue(a.cap.x);
    out_cap_power_->setValue(a.cap.y);
    in_jump_->setValue(a.cap.x);
    out_jump_->setValue(a.cap.y);

    if (a.mode == rawaccel::accel_mode::lookup && a.length > 0)
        lut_editor_->setPoints(a.data, a.length);
}

void AccelTypePanel::copyFrom(const AccelTypePanel& other) {
    rawaccel::accel_args a{};
    other.fillArgs(a);
    setArgs(a);
}
