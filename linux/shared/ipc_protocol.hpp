#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

namespace rawaccel_ipc {

static constexpr const char* SOCKET_PATH = "/var/run/rawaccel.sock";

// Wire format: [uint32 LE length][UTF-8 JSON body]

inline void ipc_send(int fd, const nlohmann::json& j) {
    std::string body = j.dump();
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {
        static_cast<uint8_t>(len & 0xFF),
        static_cast<uint8_t>((len >> 8) & 0xFF),
        static_cast<uint8_t>((len >> 16) & 0xFF),
        static_cast<uint8_t>((len >> 24) & 0xFF),
    };

    auto send_all = [fd](const void* buf, size_t n) {
        const char* p = static_cast<const char*>(buf);
        while (n > 0) {
            ssize_t r = send(fd, p, n, MSG_NOSIGNAL);
            if (r <= 0) throw std::runtime_error("ipc_send failed");
            p += r;
            n -= static_cast<size_t>(r);
        }
    };

    send_all(hdr, 4);
    send_all(body.data(), body.size());
}

inline nlohmann::json ipc_recv(int fd) {
    uint8_t hdr[4];
    size_t got = 0;
    while (got < 4) {
        ssize_t r = recv(fd, hdr + got, 4 - got, 0);
        if (r <= 0) throw std::runtime_error("ipc_recv: connection closed");
        got += static_cast<size_t>(r);
    }
    uint32_t len = static_cast<uint32_t>(hdr[0])
                 | (static_cast<uint32_t>(hdr[1]) << 8)
                 | (static_cast<uint32_t>(hdr[2]) << 16)
                 | (static_cast<uint32_t>(hdr[3]) << 24);

    std::string body(len, '\0');
    got = 0;
    while (got < len) {
        ssize_t r = recv(fd, body.data() + got, len - got, 0);
        if (r <= 0) throw std::runtime_error("ipc_recv: connection closed");
        got += static_cast<size_t>(r);
    }
    return nlohmann::json::parse(body);
}

} // namespace rawaccel_ipc
