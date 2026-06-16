#include "AccelGUI.hpp"
#include <QMetaObject>

AccelGUI::AccelGUI(QObject* parent) : QObject(parent) {
    ipc_client_ = new IpcClient(this);
    connect(ipc_client_, &IpcClient::mouseEventReceived,
            this, &AccelGUI::onMouseEvent, Qt::QueuedConnection);
    connect(ipc_client_, &IpcClient::connectionChanged,
            this, &AccelGUI::onConnectionChanged);

    chart_refresh_ = new QTimer(this);
    chart_refresh_->setInterval(100);
    chart_refresh_->setSingleShot(true);
    connect(chart_refresh_, &QTimer::timeout, this, &AccelGUI::recalculateCharts);
}

void AccelGUI::initialize() {
    // Try to load settings from daemon, then fall back to file
    auto j = ipc_client_->readSettings();
    rawaccel::modifier_settings ms;
    if (!j.is_null()) {
        ms = SettingsManager::profileFromJson(j);
        setStatus("Connected to daemon");
    } else {
        auto jfile = SettingsManager::loadJson(SettingsManager::SYSTEM_SETTINGS);
        if (jfile.is_null())
            jfile = SettingsManager::loadJson(SettingsManager::userCachePath());
        if (!jfile.is_null())
            ms = SettingsManager::profileFromJson(jfile);
        setStatus("Daemon not running — preview mode", true);
    }

    loadSettingsIntoWidgets(ms);
    ipc_client_->startSubscription();
    recalculateCharts();
}

rawaccel::modifier_settings AccelGUI::currentSettings() const {
    rawaccel::modifier_settings ms{};

    ms.prof.output_dpi          = output_dpi_opt_  ? output_dpi_opt_->value()  : rawaccel::NORMALIZED_DPI;
    ms.prof.yx_output_dpi_ratio = yx_ratio_opt_    ? yx_ratio_opt_->value()    : 1.0;
    ms.prof.degrees_rotation    = rotation_opt_    ? rotation_opt_->value()    : 0.0;
    ms.prof.degrees_snap        = snap_opt_        ? snap_opt_->value()        : 0.0;
    ms.prof.speed_max           = speed_cap_opt_   ? speed_cap_opt_->value()   : 0.0;
    ms.prof.domain_weights.x    = domain_x_opt_   ? domain_x_opt_->value()    : 1.0;
    ms.prof.domain_weights.y    = domain_y_opt_   ? domain_y_opt_->value()    : 1.0;
    ms.prof.range_weights.x     = range_x_opt_    ? range_x_opt_->value()     : 1.0;
    ms.prof.range_weights.y     = range_y_opt_    ? range_y_opt_->value()     : 1.0;
    ms.prof.speed_processor_args.whole   = whole_check_ ? whole_check_->isChecked() : true;
    ms.prof.speed_processor_args.lp_norm = lp_norm_opt_ ? lp_norm_opt_->value()    : 2.0;

    if (panel_x_) panel_x_->fillArgs(ms.prof.accel_x);
    if (panel_y_) {
        if (xy_lock_check_ && xy_lock_check_->isChecked())
            ms.prof.accel_y = ms.prof.accel_x;
        else
            panel_y_->fillArgs(ms.prof.accel_y);
    }

    rawaccel::init_data(ms);
    return ms;
}

void AccelGUI::onApply() {
    auto ms = currentSettings();
    auto j  = SettingsManager::profileToJson(ms);

    auto err = ipc_client_->writeSettings(j);
    if (err.empty()) {
        setStatus("Settings applied");
        // Save user cache
        try { SettingsManager::saveJson(SettingsManager::userCachePath(), j); } catch (...) {}
    } else {
        setStatus("Error: " + QString::fromStdString(err), true);
    }
}

void AccelGUI::onReset() {
    auto j = ipc_client_->readSettings();
    if (!j.is_null()) {
        auto ms = SettingsManager::profileFromJson(j);
        loadSettingsIntoWidgets(ms);
        recalculateCharts();
        setStatus("Reset to active settings");
    } else {
        setStatus("Cannot reset — daemon not running", true);
    }
}

void AccelGUI::onParamChanged() {
    if (!chart_refresh_->isActive())
        chart_refresh_->start();
}

void AccelGUI::onMouseEvent(double x, double y, double time_ms) {
    if (!charts_) return;
    auto ms = currentSettings();
    auto dot = calc_.calculate_dot(ms, static_cast<int>(x), static_cast<int>(y), time_ms);
    charts_->addLiveDot(dot);
}

void AccelGUI::onConnectionChanged(bool connected) {
    setStatus(connected ? "Connected to daemon" : "Daemon not running — preview mode", !connected);
}

void AccelGUI::loadSettingsIntoWidgets(const rawaccel::modifier_settings& ms) {
    const auto& prof = ms.prof;
    if (output_dpi_opt_)   output_dpi_opt_->setValue(prof.output_dpi);
    if (yx_ratio_opt_)     yx_ratio_opt_->setValue(prof.yx_output_dpi_ratio);
    if (rotation_opt_)     rotation_opt_->setValue(prof.degrees_rotation);
    if (snap_opt_)         snap_opt_->setValue(prof.degrees_snap);
    if (speed_cap_opt_)    speed_cap_opt_->setValue(prof.speed_max);
    if (domain_x_opt_)     domain_x_opt_->setValue(prof.domain_weights.x);
    if (domain_y_opt_)     domain_y_opt_->setValue(prof.domain_weights.y);
    if (range_x_opt_)      range_x_opt_->setValue(prof.range_weights.x);
    if (range_y_opt_)      range_y_opt_->setValue(prof.range_weights.y);
    if (lp_norm_opt_)      lp_norm_opt_->setValue(prof.speed_processor_args.lp_norm);
    if (whole_check_)      whole_check_->setChecked(prof.speed_processor_args.whole);
    if (panel_x_)          panel_x_->setArgs(prof.accel_x);
    if (panel_y_)          panel_y_->setArgs(prof.accel_y);
}

void AccelGUI::recalculateCharts() {
    if (!charts_) return;
    auto ms  = currentSettings();
    double d = dpi_spin_ ? dpi_spin_->value() : 1600.0;
    charts_->calculate(ms, d);
    charts_->setCombined(ms.prof.speed_processor_args.whole);
}

void AccelGUI::setStatus(const QString& msg, bool error) {
    if (status_label_) {
        status_label_->setText(msg);
        status_label_->setStyleSheet(error ? "color:red;" : "color:green;");
    }
}
