#include "DeviceManager.hpp"
#include "IpcServer.hpp"
#include "SettingsLoader.hpp"
#include "ipc_protocol.hpp"

#include <atomic>
#include <csignal>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>

static constexpr const char* SETTINGS_PATH = "/etc/rawaccel/settings.json";

static std::atomic<bool> g_running{true};

static void on_signal(int) {
    g_running = false;
}

int main() {
    if (geteuid() != 0) {
        fprintf(stderr, "rawacceld: must run as root\n");
        return 1;
    }

    std::setlocale(LC_ALL, "");
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, on_signal);
    signal(SIGINT,  on_signal);

    // Create settings directory
    mkdir("/etc/rawaccel", 0755);

    // Load initial settings
    rawaccel_linux::LoadedSettings settings;
    try {
        settings = rawaccel_linux::load_settings_file(SETTINGS_PATH);
        printf("rawacceld: loaded settings from %s\n", SETTINGS_PATH);
    } catch (const std::exception& e) {
        fprintf(stderr, "rawacceld: bad settings (%s), using defaults\n", e.what());
    }

    // Save defaults if no file exists
    {
        if (access(SETTINGS_PATH, F_OK) != 0) {
            try {
                rawaccel_linux::save_settings_file(SETTINGS_PATH, settings);
            } catch (...) {}
        }
    }

    // Current settings (shared state, protected by mutex)
    std::mutex settings_mutex;
    rawaccel_linux::LoadedSettings current_settings = settings;

    // Device manager with pre-accel event forwarding placeholder
    // (IpcServer is not created yet — we wire the callback after)
    std::atomic<rawaccel_linux::IpcServer*> ipc_ptr{nullptr};

    rawaccel_linux::EventCallback event_cb = [&ipc_ptr](double x, double y, double t) {
        auto* s = ipc_ptr.load(std::memory_order_acquire);
        if (s) s->broadcast_event(x, y, t);
    };

    rawaccel_linux::DeviceManager device_manager(event_cb);
    device_manager.apply_settings(settings);

    printf("rawacceld: managing %d mouse device(s)\n", device_manager.mouse_count());

    // IPC server
    rawaccel_linux::IpcServer ipc_server(
        // on_write
        [&](const rawaccel_linux::LoadedSettings& s) {
            {
                std::lock_guard lock(settings_mutex);
                current_settings = s;
            }
            device_manager.apply_settings(s);
            try {
                rawaccel_linux::save_settings_file(SETTINGS_PATH, s);
            } catch (const std::exception& e) {
                fprintf(stderr, "rawacceld: cannot save settings: %s\n", e.what());
            }
            printf("rawacceld: settings updated via IPC\n");
        },
        // get_current
        [&]() -> rawaccel_linux::LoadedSettings {
            std::lock_guard lock(settings_mutex);
            return current_settings;
        }
    );

    ipc_ptr.store(&ipc_server, std::memory_order_release);
    printf("rawacceld: listening on %s\n", rawaccel_ipc::SOCKET_PATH);

    // Watch settings file for live reload
    int inotify_fd = inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
    int inotify_wd = -1;
    if (inotify_fd >= 0) {
        inotify_wd = inotify_add_watch(inotify_fd, SETTINGS_PATH, IN_CLOSE_WRITE | IN_MOVED_TO);
        if (inotify_wd < 0) {
            // File may not exist yet — watch parent directory
            inotify_wd = inotify_add_watch(inotify_fd, "/etc/rawaccel", IN_CLOSE_WRITE | IN_MOVED_TO);
        }
    }

    // Main loop — wait for settings file changes or signals
    char inotify_buf[1024] __attribute__((aligned(__alignof__(struct inotify_event))));

    while (g_running) {
        fd_set fds;
        FD_ZERO(&fds);
        if (inotify_fd >= 0) FD_SET(inotify_fd, &fds);

        struct timeval tv{1, 0};
        int n = (inotify_fd >= 0)
                ? select(inotify_fd + 1, &fds, nullptr, nullptr, &tv)
                : (usleep(200000), 0);

        if (n > 0 && inotify_fd >= 0 && FD_ISSET(inotify_fd, &fds)) {
            read(inotify_fd, inotify_buf, sizeof(inotify_buf));
            try {
                auto s = rawaccel_linux::load_settings_file(SETTINGS_PATH);
                {
                    std::lock_guard lock(settings_mutex);
                    current_settings = s;
                }
                device_manager.apply_settings(s);
                printf("rawacceld: reloaded settings from file\n");
            } catch (const std::exception& e) {
                fprintf(stderr, "rawacceld: settings reload failed: %s\n", e.what());
            }
        }
    }

    ipc_ptr.store(nullptr, std::memory_order_release);
    if (inotify_wd >= 0) inotify_rm_watch(inotify_fd, inotify_wd);
    if (inotify_fd >= 0) close(inotify_fd);

    printf("rawacceld: exiting\n");
    return 0;
}
