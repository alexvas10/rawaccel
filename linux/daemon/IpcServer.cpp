#include "IpcServer.hpp"
#include "ipc_protocol.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <grp.h>

namespace rawaccel_linux {

IpcServer::IpcServer(SettingsWriteCallback on_write,
                     std::function<LoadedSettings()> get_current)
    : on_write_(std::move(on_write))
    , get_current_(std::move(get_current))
{
    unlink(rawaccel_ipc::SOCKET_PATH);

    server_fd_ = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server_fd_ < 0)
        throw std::runtime_error("Cannot create IPC socket");

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, rawaccel_ipc::SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("Cannot bind IPC socket");

    // Permissions: group 'input' can connect
    chmod(rawaccel_ipc::SOCKET_PATH, 0660);
    struct group* grp = getgrnam("input");
    if (grp) chown(rawaccel_ipc::SOCKET_PATH, 0, grp->gr_gid);

    if (listen(server_fd_, 16) < 0)
        throw std::runtime_error("Cannot listen on IPC socket");

    running_ = true;
    accept_thread_ = std::thread(&IpcServer::run_accept_loop, this);
}

IpcServer::~IpcServer() {
    running_ = false;
    shutdown(server_fd_, SHUT_RDWR);
    close(server_fd_);
    if (accept_thread_.joinable()) accept_thread_.join();

    std::lock_guard lock(subscribers_mutex_);
    for (int fd : subscriber_fds_) close(fd);
    subscriber_fds_.clear();

    unlink(rawaccel_ipc::SOCKET_PATH);
}

void IpcServer::broadcast_event(double x, double y, double time_ms) {
    nlohmann::json msg;
    msg["type"] = "EVENT";
    msg["x"]    = x;
    msg["y"]    = y;
    msg["time_ms"] = time_ms;

    std::string body = msg.dump();
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {
        static_cast<uint8_t>(len & 0xFF),
        static_cast<uint8_t>((len >> 8) & 0xFF),
        static_cast<uint8_t>((len >> 16) & 0xFF),
        static_cast<uint8_t>((len >> 24) & 0xFF),
    };

    std::vector<int> dead;
    {
        std::lock_guard lock(subscribers_mutex_);
        for (int fd : subscriber_fds_) {
            // Non-blocking: if the GUI's receive buffer is full, drop this event
            // rather than stalling the daemon's per-device thread.
            ssize_t r = send(fd, hdr, 4, MSG_NOSIGNAL | MSG_DONTWAIT);
            if (r == 4)
                send(fd, body.data(), body.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
            else if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
                dead.push_back(fd); // real error (EPIPE, ECONNRESET) — subscriber gone
        }
        for (int fd : dead) {
            close(fd);
            subscriber_fds_.erase(std::remove(subscriber_fds_.begin(),
                                               subscriber_fds_.end(), fd),
                                   subscriber_fds_.end());
        }
    }
}

void IpcServer::run_accept_loop() {
    while (running_) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_fd_, &fds);
        struct timeval tv{1, 0};
        if (select(server_fd_ + 1, &fds, nullptr, nullptr, &tv) <= 0) continue;

        int client = accept4(server_fd_, nullptr, nullptr, SOCK_CLOEXEC);
        if (client < 0) continue;

        // Handle each client in a detached thread
        std::thread([this, client]{ handle_client(client); }).detach();
    }
}

void IpcServer::handle_client(int fd) {
    try {
        auto msg = rawaccel_ipc::ipc_recv(fd);
        std::string cmd = msg.value("cmd", "");

        if (cmd == "READ") {
            auto settings = get_current_();
            nlohmann::json resp;
            resp["type"]     = "SETTINGS";
            resp["settings"] = nlohmann::json::parse(settings_to_json(settings));
            rawaccel_ipc::ipc_send(fd, resp);
            close(fd);

        } else if (cmd == "WRITE") {
            try {
                auto j = msg["settings"].dump();
                auto s = settings_from_json(j);
                on_write_(s);

                nlohmann::json resp;
                resp["type"]     = "SETTINGS";
                resp["settings"] = msg["settings"];
                rawaccel_ipc::ipc_send(fd, resp);
            } catch (const std::exception& e) {
                nlohmann::json resp;
                resp["type"]    = "ERROR";
                resp["message"] = e.what();
                rawaccel_ipc::ipc_send(fd, resp);
            }
            close(fd);

        } else if (cmd == "SUBSCRIBE") {
            add_subscriber(fd);
            // fd stays open; closed on disconnect or daemon shutdown
        } else {
            close(fd);
        }
    } catch (...) {
        close(fd);
    }
}

void IpcServer::add_subscriber(int fd) {
    std::lock_guard lock(subscribers_mutex_);
    subscriber_fds_.push_back(fd);
}

void IpcServer::remove_subscriber(int fd) {
    std::lock_guard lock(subscribers_mutex_);
    subscriber_fds_.erase(std::remove(subscriber_fds_.begin(),
                                       subscriber_fds_.end(), fd),
                           subscriber_fds_.end());
    close(fd);
}

} // namespace rawaccel_linux
