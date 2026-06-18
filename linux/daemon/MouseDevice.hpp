#pragma once

#include "shared/compat.hpp"
#include "rawaccel.hpp"
#include "VirtualMouse.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <sys/eventfd.h>

struct libevdev;

namespace rawaccel_linux {

// Called for each pre-acceleration event packet (for chart overlay).
using EventCallback = std::function<void(double x, double y, double time_ms)>;

class MouseDevice {
public:
    explicit MouseDevice(const std::string& path,
                         VirtualMouse& virtual_mouse,
                         EventCallback event_cb = nullptr);
    ~MouseDevice();

    MouseDevice(const MouseDevice&) = delete;
    MouseDevice& operator=(const MouseDevice&) = delete;

    bool is_mouse() const { return is_mouse_; }
    const std::string& path() const { return path_; }
    std::string device_name() const;

    void update_settings(const rawaccel::modifier_settings& settings,
                         const rawaccel::device_config& config);

    void stop();

private:
    std::string path_;
    int fd_ = -1;
    struct libevdev* dev_ = nullptr;
    bool is_mouse_ = false;

    VirtualMouse& virtual_mouse_;
    EventCallback event_cb_;

    mutable std::shared_mutex settings_mutex_;
    rawaccel::modifier_settings settings_;
    rawaccel::device_config dev_cfg_;
    rawaccel::modifier modifier_;
    rawaccel::speed_processor speed_proc_;

    // Fractional carry-over (sub-count accumulator)
    double frac_x_ = 0.0;
    double frac_y_ = 0.0;

    std::thread thread_;
    std::atomic<bool> running_{false};
    int wakeup_fd_ = -1;

    void run_loop();
    void process_event(int raw_x, int raw_y, double delta_ms);
    void reinit_modifier();
};

} // namespace rawaccel_linux
