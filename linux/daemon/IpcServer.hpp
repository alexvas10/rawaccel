#pragma once

#include "SettingsLoader.hpp"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace rawaccel_linux {

using SettingsWriteCallback = std::function<void(const LoadedSettings&)>;

class IpcServer {
public:
    // on_write is called when a GUI sends new settings.
    // get_current returns the current active settings for READ requests.
    IpcServer(SettingsWriteCallback on_write,
              std::function<LoadedSettings()> get_current);
    ~IpcServer();

    IpcServer(const IpcServer&) = delete;
    IpcServer& operator=(const IpcServer&) = delete;

    // Broadcast pre-acceleration mouse event to all SUBSCRIBE clients.
    void broadcast_event(double x, double y, double time_ms);

private:
    int server_fd_ = -1;
    SettingsWriteCallback on_write_;
    std::function<LoadedSettings()> get_current_;

    std::thread accept_thread_;
    std::atomic<bool> running_{false};

    mutable std::mutex subscribers_mutex_;
    std::vector<int> subscriber_fds_;

    void run_accept_loop();
    void handle_client(int client_fd);
    void add_subscriber(int fd);
    void remove_subscriber(int fd);
};

} // namespace rawaccel_linux
