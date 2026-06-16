#pragma once

#include "MouseDevice.hpp"
#include "SettingsLoader.hpp"
#include "VirtualMouse.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace rawaccel_linux {

class DeviceManager {
public:
    explicit DeviceManager(EventCallback event_cb = nullptr);
    ~DeviceManager();

    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;

    void apply_settings(const LoadedSettings& s);
    int mouse_count() const;

private:
    std::unique_ptr<VirtualMouse> virtual_mouse_;
    EventCallback event_cb_;

    mutable std::mutex devices_mutex_;
    std::unordered_map<std::string, std::unique_ptr<MouseDevice>> devices_;

    LoadedSettings current_settings_;
    mutable std::mutex settings_mutex_;

    int inotify_fd_ = -1;
    int inotify_wd_ = -1;
    std::thread hotplug_thread_;
    bool running_ = false;

    void enumerate_devices();
    void add_device(const std::string& path);
    void remove_device(const std::string& path);
    void run_hotplug_loop();

    rawaccel::device_config config_for_device(const std::string& dev_name) const;
};

} // namespace rawaccel_linux
