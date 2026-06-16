#pragma once

#include "shared/compat.hpp"
#include "rawaccel.hpp"
#include "IpcClient.hpp"
#include "SettingsManager.hpp"
#include "Calculator.hpp"
#include "Charts/AccelCharts.hpp"
#include "AccelTypePanel.hpp"
#include "Options/OptionWidget.hpp"
#include "Options/LockableOption.hpp"

#include <QObject>
#include <QSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QLabel>

// Central coordinator: reads GUI widgets → calls Calculator → updates charts.
// Equivalent to C# AccelGUI.cs.
class AccelGUI : public QObject {
    Q_OBJECT
public:
    explicit AccelGUI(QObject* parent = nullptr);

    // Wire up to the widgets created in MainWindow
    void setCharts(AccelCharts* charts)           { charts_ = charts; }
    void setDpiSpin(QSpinBox* s)                  { dpi_spin_ = s; }
    void setPollRateSpin(QSpinBox* s)              { poll_spin_ = s; }
    void setOutputDpiOpt(OptionWidget* o)          { output_dpi_opt_ = o; }
    void setYxRatioOpt(LockableOption* o)          { yx_ratio_opt_ = o; }
    void setRotationOpt(OptionWidget* o)           { rotation_opt_ = o; }
    void setSnapOpt(OptionWidget* o)               { snap_opt_ = o; }
    void setSpeedCapOpt(OptionWidget* o)           { speed_cap_opt_ = o; }
    void setLpNormOpt(OptionWidget* o)             { lp_norm_opt_ = o; }
    void setDomainXOpt(OptionWidget* o)            { domain_x_opt_ = o; }
    void setDomainYOpt(OptionWidget* o)            { domain_y_opt_ = o; }
    void setRangeXOpt(OptionWidget* o)             { range_x_opt_ = o; }
    void setRangeYOpt(OptionWidget* o)             { range_y_opt_ = o; }
    void setWholeCheck(QCheckBox* c)               { whole_check_ = c; }
    void setAccelPanelX(AccelTypePanel* p)         { panel_x_ = p; }
    void setAccelPanelY(AccelTypePanel* p)         { panel_y_ = p; }
    void setXYLockCheck(QCheckBox* c)              { xy_lock_check_ = c; }
    void setStatusLabel(QLabel* l)                 { status_label_ = l; }

    // Call after all widgets are wired.
    void initialize();

    // Current modifier settings (used for chart calculation)
    rawaccel::modifier_settings currentSettings() const;

public slots:
    void onApply();
    void onReset();
    void onParamChanged();
    void onMouseEvent(double x, double y, double time_ms);
    void onConnectionChanged(bool connected);

private:
    AccelCharts*    charts_        = nullptr;
    QSpinBox*       dpi_spin_      = nullptr;
    QSpinBox*       poll_spin_     = nullptr;
    OptionWidget*   output_dpi_opt_= nullptr;
    LockableOption* yx_ratio_opt_  = nullptr;
    OptionWidget*   rotation_opt_  = nullptr;
    OptionWidget*   snap_opt_      = nullptr;
    OptionWidget*   speed_cap_opt_ = nullptr;
    OptionWidget*   lp_norm_opt_   = nullptr;
    OptionWidget*   domain_x_opt_  = nullptr;
    OptionWidget*   domain_y_opt_  = nullptr;
    OptionWidget*   range_x_opt_   = nullptr;
    OptionWidget*   range_y_opt_   = nullptr;
    QCheckBox*      whole_check_   = nullptr;
    AccelTypePanel* panel_x_       = nullptr;
    AccelTypePanel* panel_y_       = nullptr;
    QCheckBox*      xy_lock_check_ = nullptr;
    QLabel*         status_label_  = nullptr;

    IpcClient*      ipc_client_    = nullptr;
    Calculator      calc_;
    QTimer*         chart_refresh_ = nullptr;

    void loadSettingsIntoWidgets(const rawaccel::modifier_settings& settings);
    void recalculateCharts();
    void setStatus(const QString& msg, bool error = false);
};
