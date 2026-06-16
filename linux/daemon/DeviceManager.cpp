#include "DeviceManager.hpp"

#include <cstring>
#include <cwchar>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>
#include <sys/inotify.h>
#include <linux/input.h>

#include <libevdev/libevdev.h>

namespace rawaccel_linux {

static std::string wchar_to_utf8(const wchar_t* ws) {
    char buf[1024] = {};
    std::wcstombs(buf, ws, sizeof(buf) - 1);
    return std::string(buf);
}

DeviceManager::DeviceManager(EventCallback event_cb)
    : event_cb_(std::move(event_cb))
{
    virtual_mouse_ = std::make_unique<VirtualMouse>("rawaccel");

    rawaccel::init_data(current_settings_.modifier);

    enumerate_devices();

    // Watch /dev/input for hotplug
    inotify_fd_ = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    if (inotify_fd_ >= 0) {
        inotify_wd_ = inotify_add_watch(inotify_fd_, "/dev/input",
                                         IN_CREATE | IN_DELETE | IN_ATTRIB);
        running_ = true;
        hotplug_thread_ = std::thread(&DeviceManager::run_hotplug_loop, this);
    }
}

DeviceManager::~DeviceManager() {
    running_ = false;
    if (hotplug_thread_.joinable()) hotplug_thread_.join();
    if (inotify_wd_ >= 0) inotify_rm_watch(inotify_fd_, inotify_wd_);
    if (inotify_fd_ >= 0) close(inotify_fd_);

    // Stop all devices before destroying virtual mouse
    std::lock_guard lock(devices_mutex_);
    devices_.clear();
}

void DeviceManager::apply_settings(const LoadedSettings& s) {
    {
        std::lock_guard lock(settings_mutex_);
        current_settings_ = s;
    }

    std::lock_guard lock(devices_mutex_);
    for (auto& [path, dev] : devices_) {
        if (dev) {
            auto cfg = config_for_device(dev->device_name());
            dev->update_settings(s.modifier, cfg);
        }
    }
}

int DeviceManager::mouse_count() const {
    std::lock_guard lock(devices_mutex_);
    return static_cast<int>(devices_.size());
}

void DeviceManager::enumerate_devices() {
    glob_t g{};
    if (glob("/dev/input/event*", GLOB_NOSORT, nullptr, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; ++i)
            add_device(g.gl_pathv[i]);
    }
    globfree(&g);
}

void DeviceManager::add_device(const std::string& path) {
    rawaccel::modifier_settings ms;
    rawaccel::device_config cfg;
    {
        std::lock_guard lock(settings_mutex_);
        ms  = current_settings_.modifier;
        // cfg determined below after we know device name
    }

    auto dev = std::make_unique<MouseDevice>(path, *virtual_mouse_, event_cb_);
    if (!dev->is_mouse()) return;

    {
        std::lock_guard lock(settings_mutex_);
        cfg = config_for_device(dev->device_name());
    }
    dev->update_settings(ms, cfg);

    std::lock_guard lock(devices_mutex_);
    devices_[path] = std::move(dev);
}

void DeviceManager::remove_device(const std::string& path) {
    std::lock_guard lock(devices_mutex_);
    devices_.erase(path);
}

void DeviceManager::run_hotplug_loop() {
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(inotify_fd_, &fds);
        struct timeval tv{1, 0};
        if (select(inotify_fd_ + 1, &fds, nullptr, nullptr, &tv) <= 0) continue;

        ssize_t len = read(inotify_fd_, buf, sizeof(buf));
        if (len <= 0) continue;

        for (char* p = buf; p < buf + len; ) {
            auto* ev = reinterpret_cast<struct inotify_event*>(p);
            if (ev->len > 0) {
                std::string name(ev->name);
                if (name.substr(0, 5) == "event") {
                    std::string full = "/dev/input/" + name;
                    if (ev->mask & (IN_CREATE | IN_ATTRIB)) {
                        usleep(100000); // Brief delay for device to settle
                        add_device(full);
                    } else if (ev->mask & IN_DELETE) {
                        remove_device(full);
                    }
                }
            }
            p += sizeof(struct inotify_event) + ev->len;
        }
    }
}

rawaccel::device_config DeviceManager::config_for_device(const std::string& dev_name) const {
    for (const auto& ds : current_settings_.devices) {
        std::string n = wchar_to_utf8(ds.name);
        if (n == dev_name)
            return ds.config;
    }
    return current_settings_.default_device_cfg;
}

} // namespace rawaccel_linux
