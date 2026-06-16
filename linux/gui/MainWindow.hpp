#pragma once

#include "AccelGUI.hpp"
#include "Charts/AccelCharts.hpp"
#include "AccelTypePanel.hpp"
#include "Options/OptionWidget.hpp"
#include "Options/LockableOption.hpp"

#include <QMainWindow>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QTimer>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onApplyClicked();
    void onResetClicked();
    void onToggleVelocityGain(bool checked);
    void onWholeChanged(bool checked);

private:
    AccelGUI*       accel_gui_;
    AccelCharts*    charts_;

    QSpinBox*       dpi_spin_;
    QSpinBox*       poll_spin_;
    OptionWidget*   output_dpi_opt_;
    LockableOption* yx_ratio_opt_;
    OptionWidget*   rotation_opt_;
    OptionWidget*   snap_opt_;
    OptionWidget*   speed_cap_opt_;

    // Anisotropy section
    QGroupBox*      aniso_group_;
    QCheckBox*      whole_check_;
    OptionWidget*   lp_norm_opt_;
    OptionWidget*   domain_x_opt_;
    OptionWidget*   domain_y_opt_;
    OptionWidget*   range_x_opt_;
    OptionWidget*   range_y_opt_;

    AccelTypePanel* panel_x_;
    AccelTypePanel* panel_y_;
    QCheckBox*      xy_lock_check_;

    QPushButton*    apply_btn_;
    QPushButton*    reset_btn_;
    QLabel*         status_label_;

    QTimer*         write_delay_timer_;

    QWidget* buildSettingsPanel();
    void setupMenuBar();
};
