#include "MouseDevice.hpp"

#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <libevdev/libevdev.h>
#include <linux/input.h>

namespace rawaccel_linux {

MouseDevice::MouseDevice(const std::string& path,
                         VirtualMouse& virtual_mouse,
                         EventCallback event_cb)
    : path_(path)
    , virtual_mouse_(virtual_mouse)
    , event_cb_(std::move(event_cb))
{
    fd_ = open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd_ < 0) return;

    if (libevdev_new_from_fd(fd_, &dev_) < 0) {
        close(fd_);
        fd_ = -1;
        return;
    }

    // Only process relative-input devices that have X and Y axes
    is_mouse_ = libevdev_has_event_type(dev_, EV_REL)
             && libevdev_has_event_code(dev_, EV_REL, REL_X)
             && libevdev_has_event_code(dev_, EV_REL, REL_Y);

    // Exclude our own virtual output device — otherwise we'd grab and read back
    // the events we just emitted, feeding back into itself forever.
    if (is_mouse_
        && libevdev_get_id_vendor(dev_)  == VirtualMouse::VENDOR_ID
        && libevdev_get_id_product(dev_) == VirtualMouse::PRODUCT_ID) {
        is_mouse_ = false;
    }

    if (!is_mouse_) return;

    // Grab device exclusively so X11/Wayland see only our virtual output
    if (libevdev_grab(dev_, LIBEVDEV_GRAB) < 0) {
        // Non-fatal — some devices (touchpads acting as mice) may refuse
    }

    rawaccel::init_data(settings_);
    modifier_ = rawaccel::modifier(settings_);
    speed_proc_.init(settings_.prof.speed_processor_args);

    wakeup_fd_ = eventfd(0, EFD_CLOEXEC);
    running_ = true;
    thread_ = std::thread(&MouseDevice::run_loop, this);
}

MouseDevice::~MouseDevice() {
    stop();
    if (wakeup_fd_ >= 0) close(wakeup_fd_);
    if (dev_) {
        libevdev_grab(dev_, LIBEVDEV_UNGRAB);
        libevdev_free(dev_);
    }
    if (fd_ >= 0) close(fd_);
}

std::string MouseDevice::device_name() const {
    if (dev_) return libevdev_get_name(dev_);
    return {};
}

void MouseDevice::update_settings(const rawaccel::modifier_settings& settings,
                                   const rawaccel::device_config& config)
{
    std::unique_lock lock(settings_mutex_);
    settings_ = settings;
    dev_cfg_  = config;
    reinit_modifier();
}

void MouseDevice::stop() {
    running_ = false;
    if (wakeup_fd_ >= 0) {
        uint64_t val = 1;
        write(wakeup_fd_, &val, sizeof(val));
    }
    if (thread_.joinable()) thread_.join();
}

void MouseDevice::reinit_modifier() {
    rawaccel::init_data(settings_);
    modifier_ = rawaccel::modifier(settings_);
    speed_proc_.init(settings_.prof.speed_processor_args);
    frac_x_ = 0.0;
    frac_y_ = 0.0;
}

void MouseDevice::run_loop() {
    struct input_event ev{};
    int acc_x = 0, acc_y = 0;
    struct timeval last_time{};
    bool has_last = false;
    bool in_sync = false;

    struct pollfd pfds[2]{};
    pfds[0].fd = fd_;
    pfds[0].events = POLLIN;
    pfds[1].fd = wakeup_fd_;
    pfds[1].events = POLLIN;

    while (running_) {
        // Wait for data on the mouse fd or a stop() wakeup.
        // O_NONBLOCK on the fd means libevdev_next_event(NORMAL) returns -EAGAIN
        // when libevdev's internal buffer is exhausted, so we can safely drain
        // all buffered events in the inner loop below without blocking.
        if (!in_sync) {
            if (poll(pfds, 2, -1) <= 0) continue;
            if (pfds[1].revents & POLLIN) break; // stop() was called
            if (!(pfds[0].revents & POLLIN)) continue;
        }

        // Drain all events libevdev has buffered (it reads up to 64 at a time
        // from the kernel, so one poll() wakeup may cover many events).
        while (running_) {
            int flags = in_sync ? LIBEVDEV_READ_FLAG_SYNC : LIBEVDEV_READ_FLAG_NORMAL;
            int rc = libevdev_next_event(dev_, flags, &ev);

            if (rc == LIBEVDEV_READ_STATUS_SYNC) {
                in_sync = true;
                continue;
            }

            if (rc == -EAGAIN) {
                in_sync = false;
                break; // Buffer exhausted — go back to poll()
            }

            if (rc < 0) goto done; // Device disconnected

            if (ev.type == EV_REL) {
                if (ev.code == REL_X) acc_x += ev.value;
                else if (ev.code == REL_Y) acc_y += ev.value;
                else {
                    // Wheel and other relative axes pass through verbatim
                    virtual_mouse_.emit_event(ev.type, ev.code, ev.value);
                }
            } else if (ev.type == EV_KEY) {
                // All button events pass through verbatim
                virtual_mouse_.emit_event(ev.type, ev.code, ev.value);
            } else if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                if (acc_x == 0 && acc_y == 0) {
                    // No motion this frame, but flush any pending button/wheel
                    // events emitted above — otherwise they sit unsynced until
                    // the next motion event happens to flush them.
                    virtual_mouse_.sync();
                    has_last = false;
                    continue;
                }

                double delta_ms = 1.0; // Default to 1ms if no timing info
                if (has_last) {
                    delta_ms = (ev.time.tv_sec - last_time.tv_sec) * 1000.0
                             + (ev.time.tv_usec - last_time.tv_usec) / 1000.0;
                }
                last_time = ev.time;
                has_last = true;

                // Clamp timing
                {
                    std::shared_lock lock(settings_mutex_);
                    double mn = dev_cfg_.clamp.min;
                    double mx = dev_cfg_.clamp.max;
                    if (mn <= 0) mn = rawaccel::DEFAULT_TIME_MIN;
                    if (mx <= 0) mx = rawaccel::DEFAULT_TIME_MAX;
                    if (delta_ms < mn) delta_ms = mn;
                    if (delta_ms > mx) delta_ms = mx;
                }

                if (event_cb_) event_cb_(acc_x, acc_y, delta_ms);

                process_event(acc_x, acc_y, delta_ms);
                acc_x = 0;
                acc_y = 0;
            }
        } // end drain loop
    } // end running_ loop
done:
    ;
}

void MouseDevice::process_event(int raw_x, int raw_y, double delta_ms) {
    vec2d in_out{static_cast<double>(raw_x),
                 static_cast<double>(raw_y)};

    double dpi_factor = 1.0;
    {
        std::shared_lock lock(settings_mutex_);
        if (dev_cfg_.dpi > 0)
            dpi_factor = static_cast<double>(dev_cfg_.dpi) / rawaccel::NORMALIZED_DPI;

        if (!dev_cfg_.disable)
            modifier_.modify(in_out, speed_proc_, settings_, dpi_factor, delta_ms);
    }

    frac_x_ += in_out.x;
    frac_y_ += in_out.y;

    int emit_x = static_cast<int>(frac_x_);
    int emit_y = static_cast<int>(frac_y_);
    frac_x_ -= emit_x;
    frac_y_ -= emit_y;

    if (emit_x || emit_y)
        virtual_mouse_.emit_motion(emit_x, emit_y);
}

} // namespace rawaccel_linux
