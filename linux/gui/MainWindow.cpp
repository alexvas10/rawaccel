#include "MainWindow.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QSplitter>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("rawaccel");
    resize(1200, 700);

    write_delay_timer_ = new QTimer(this);
    write_delay_timer_->setSingleShot(true);

    // Build central layout
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* hl = new QHBoxLayout(central);
    hl->setContentsMargins(4, 4, 4, 4);
    hl->setSpacing(6);

    // Left: scrollable settings panel
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFixedWidth(400);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(buildSettingsPanel());
    hl->addWidget(scroll);

    // Right: charts
    charts_ = new AccelCharts(this);
    hl->addWidget(charts_, 1);

    // Status bar
    status_label_ = new QLabel("Initializing...", this);
    statusBar()->addWidget(status_label_);

    setupMenuBar();

    // Wire AccelGUI coordinator
    accel_gui_ = new AccelGUI(this);
    accel_gui_->setCharts(charts_);
    accel_gui_->setDpiSpin(dpi_spin_);
    accel_gui_->setPollRateSpin(poll_spin_);
    accel_gui_->setOutputDpiOpt(output_dpi_opt_);
    accel_gui_->setYxRatioOpt(yx_ratio_opt_);
    accel_gui_->setRotationOpt(rotation_opt_);
    accel_gui_->setSnapOpt(snap_opt_);
    accel_gui_->setSpeedCapOpt(speed_cap_opt_);
    accel_gui_->setLpNormOpt(lp_norm_opt_);
    accel_gui_->setDomainXOpt(domain_x_opt_);
    accel_gui_->setDomainYOpt(domain_y_opt_);
    accel_gui_->setRangeXOpt(range_x_opt_);
    accel_gui_->setRangeYOpt(range_y_opt_);
    accel_gui_->setWholeCheck(whole_check_);
    accel_gui_->setAccelPanelX(panel_x_);
    accel_gui_->setAccelPanelY(panel_y_);
    accel_gui_->setXYLockCheck(xy_lock_check_);
    accel_gui_->setStatusLabel(status_label_);

    // Connect param changes to chart refresh
    for (auto* w : {output_dpi_opt_, rotation_opt_, snap_opt_,
                    speed_cap_opt_, lp_norm_opt_, domain_x_opt_,
                    domain_y_opt_, range_x_opt_, range_y_opt_})
        connect(w, &OptionWidget::valueChanged, accel_gui_, &AccelGUI::onParamChanged);

    connect(yx_ratio_opt_, &LockableOption::valueChanged, accel_gui_, &AccelGUI::onParamChanged);
    connect(whole_check_, &QCheckBox::toggled, this, &MainWindow::onWholeChanged);
    connect(panel_x_, &AccelTypePanel::changed, accel_gui_, &AccelGUI::onParamChanged);
    connect(panel_y_, &AccelTypePanel::changed, accel_gui_, &AccelGUI::onParamChanged);
    connect(dpi_spin_, QOverload<int>::of(&QSpinBox::valueChanged), accel_gui_, &AccelGUI::onParamChanged);

    connect(apply_btn_, &QPushButton::clicked, this, &MainWindow::onApplyClicked);
    connect(reset_btn_, &QPushButton::clicked, this, &MainWindow::onResetClicked);

    // Write delay: re-enable buttons after 1 second (matches Windows behavior)
    connect(write_delay_timer_, &QTimer::timeout, this, [this]{
        apply_btn_->setEnabled(true);
        reset_btn_->setEnabled(true);
        apply_btn_->setText("Apply");
    });

    // Initialize after event loop starts
    QMetaObject::invokeMethod(this, [this]{ accel_gui_->initialize(); }, Qt::QueuedConnection);
}

QWidget* MainWindow::buildSettingsPanel() {
    auto* panel = new QWidget;
    auto* vl    = new QVBoxLayout(panel);
    vl->setContentsMargins(4, 4, 4, 4);
    vl->setSpacing(6);

    // DPI / Poll Rate row
    auto* chart_scale_group = new QGroupBox("Chart Scale", panel);
    auto* chl = new QHBoxLayout(chart_scale_group);
    dpi_spin_ = new QSpinBox(panel);
    dpi_spin_->setRange(100, 32000);
    dpi_spin_->setValue(1600);
    dpi_spin_->setPrefix("DPI: ");
    poll_spin_ = new QSpinBox(panel);
    poll_spin_->setRange(125, 8000);
    poll_spin_->setValue(1000);
    poll_spin_->setPrefix("Hz: ");
    chl->addWidget(dpi_spin_);
    chl->addWidget(poll_spin_);
    vl->addWidget(chart_scale_group);

    // Sensitivity section
    auto* sens_group = new QGroupBox("Sensitivity", panel);
    auto* sl = new QVBoxLayout(sens_group);
    output_dpi_opt_ = new OptionWidget("Output DPI", panel);
    output_dpi_opt_->setValue(1000.0);
    output_dpi_opt_->setRange(1, 100000);
    yx_ratio_opt_   = new LockableOption("Y/X Ratio",  panel);
    yx_ratio_opt_->setValue(1.0);
    rotation_opt_   = new OptionWidget("Rotation °",   panel);
    snap_opt_       = new OptionWidget("Snap Angle °", panel);
    snap_opt_->setRange(0, 45);
    speed_cap_opt_  = new OptionWidget("Speed Cap",    panel);
    speed_cap_opt_->setRange(0, 1e9);
    for (auto* w : {output_dpi_opt_, rotation_opt_, snap_opt_, speed_cap_opt_})
        sl->addWidget(w);
    sl->addWidget(yx_ratio_opt_);
    vl->addWidget(sens_group);

    // Anisotropy section (collapsible group box)
    aniso_group_ = new QGroupBox("Anisotropy", panel);
    aniso_group_->setCheckable(true);
    aniso_group_->setChecked(false);
    auto* al = new QVBoxLayout(aniso_group_);
    whole_check_ = new QCheckBox("Whole / Combined (uncheck for By Component)", panel);
    whole_check_->setChecked(true);
    lp_norm_opt_ = new OptionWidget("Lp Norm",  panel);
    lp_norm_opt_->setValue(2.0);
    lp_norm_opt_->setRange(0.1, 32.0);
    domain_x_opt_ = new OptionWidget("Domain X", panel);
    domain_x_opt_->setValue(1.0);
    domain_y_opt_ = new OptionWidget("Domain Y", panel);
    domain_y_opt_->setValue(1.0);
    range_x_opt_  = new OptionWidget("Range X",  panel);
    range_x_opt_->setValue(1.0);
    range_y_opt_  = new OptionWidget("Range Y",  panel);
    range_y_opt_->setValue(1.0);
    for (auto* w : {whole_check_})
        al->addWidget(w);
    for (auto* w : {lp_norm_opt_, domain_x_opt_, domain_y_opt_, range_x_opt_, range_y_opt_})
        al->addWidget(w);
    vl->addWidget(aniso_group_);

    // X/Y lock
    xy_lock_check_ = new QCheckBox("Use same settings for X and Y", panel);
    xy_lock_check_->setChecked(true);
    vl->addWidget(xy_lock_check_);

    // Acceleration panels
    panel_x_ = new AccelTypePanel("Acceleration", panel);
    vl->addWidget(panel_x_);

    panel_y_ = new AccelTypePanel("Y Acceleration", panel);
    panel_y_->setVisible(false);
    vl->addWidget(panel_y_);

    connect(xy_lock_check_, &QCheckBox::toggled, panel_y_, [this](bool locked){
        panel_y_->setVisible(!locked);
    });

    // Apply / Reset buttons
    auto* btn_layout = new QHBoxLayout;
    apply_btn_ = new QPushButton("Apply", panel);
    apply_btn_->setDefault(true);
    reset_btn_ = new QPushButton("Reset", panel);
    btn_layout->addWidget(apply_btn_);
    btn_layout->addWidget(reset_btn_);
    vl->addLayout(btn_layout);

    vl->addStretch();
    return panel;
}

void MainWindow::setupMenuBar() {
    auto* view_menu = menuBar()->addMenu("View");

    auto* vel_gain_act = view_menu->addAction("Show Velocity && Gain Charts");
    vel_gain_act->setCheckable(true);
    connect(vel_gain_act, &QAction::toggled, this, &MainWindow::onToggleVelocityGain);

    auto* help_menu = menuBar()->addMenu("Help");
    auto* about_act = help_menu->addAction("About");
    connect(about_act, &QAction::triggered, this, [this]{
        QMessageBox::about(this, "rawaccel",
            "rawaccel for Linux\n\n"
            "Port of the Windows rawaccel mouse acceleration tool.\n"
            "Settings files are compatible with the Windows version.");
    });
}

void MainWindow::onApplyClicked() {
    apply_btn_->setEnabled(false);
    reset_btn_->setEnabled(false);
    apply_btn_->setText("Delay…");
    accel_gui_->onApply();
    write_delay_timer_->start(1000);
}

void MainWindow::onResetClicked() {
    accel_gui_->onReset();
}

void MainWindow::onToggleVelocityGain(bool checked) {
    charts_->setShowVelocityAndGain(checked);
}

void MainWindow::onWholeChanged(bool checked) {
    lp_norm_opt_->setVisible(!checked);
    domain_x_opt_->setVisible(!checked);
    domain_y_opt_->setVisible(!checked);
    accel_gui_->onParamChanged();
}
