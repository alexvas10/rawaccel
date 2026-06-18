#include "IpcClient.hpp"

#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

IpcClient::IpcClient(QObject* parent) : QObject(parent) {
    reconnect_timer_ = new QTimer(this);
    reconnect_timer_->setInterval(5000);
    connect(reconnect_timer_, &QTimer::timeout, this, &IpcClient::tryConnect);

    sub_socket_ = new QLocalSocket(this);
    connect(sub_socket_, &QLocalSocket::readyRead,    this, &IpcClient::onSubDataReady);
    connect(sub_socket_, &QLocalSocket::disconnected, this, &IpcClient::onSubDisconnected);
}

IpcClient::~IpcClient() {
    stopSubscription();
}

bool IpcClient::isConnected() const {
    return sub_socket_->state() == QLocalSocket::ConnectedState;
}

bool IpcClient::do_request(const nlohmann::json& req, nlohmann::json& resp) {
    // Use a plain POSIX socket instead of QLocalSocket.
    // Qt's waitForConnected/waitForReadyRead internally call processEvents(),
    // which allows the reconnect timer and sub_socket_ disconnect signals to
    // fire reentrant mid-request — corrupting the status display. POSIX calls
    // block the thread directly without touching the Qt event loop.
    int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return false;

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd); return false;
    }

    // Send length-prefixed JSON request
    std::string body = req.dump();
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {
        uint8_t(len & 0xFF), uint8_t((len >> 8) & 0xFF),
        uint8_t((len >> 16) & 0xFF), uint8_t((len >> 24) & 0xFF)
    };
    auto send_all = [fd](const void* buf, size_t n) -> bool {
        const char* p = static_cast<const char*>(buf);
        while (n > 0) {
            ssize_t r = ::send(fd, p, n, MSG_NOSIGNAL);
            if (r <= 0) return false;
            p += r; n -= static_cast<size_t>(r);
        }
        return true;
    };
    if (!send_all(hdr, 4) || !send_all(body.data(), body.size())) {
        ::close(fd); return false;
    }

    // Read length-prefixed JSON response
    auto recv_all = [fd](void* buf, size_t n) -> bool {
        char* p = static_cast<char*>(buf);
        while (n > 0) {
            ssize_t r = ::recv(fd, p, n, 0);
            if (r <= 0) return false;
            p += r; n -= static_cast<size_t>(r);
        }
        return true;
    };
    uint8_t rhdr[4];
    if (!recv_all(rhdr, 4)) { ::close(fd); return false; }
    uint32_t rlen = uint32_t(rhdr[0]) | (uint32_t(rhdr[1]) << 8)
                  | (uint32_t(rhdr[2]) << 16) | (uint32_t(rhdr[3]) << 24);
    std::string rbody(rlen, '\0');
    if (!recv_all(rbody.data(), rlen)) { ::close(fd); return false; }

    try {
        resp = nlohmann::json::parse(rbody);
        ::close(fd);
        return true;
    } catch (...) {
        ::close(fd);
        return false;
    }
}

nlohmann::json IpcClient::readSettings() {
    nlohmann::json req{{"cmd","READ"}}, resp;
    if (!do_request(req, resp)) return {};
    return resp.value("settings", nlohmann::json{});
}

std::string IpcClient::writeSettings(const nlohmann::json& settings) {
    nlohmann::json req{{"cmd","WRITE"},{"settings",settings}}, resp;
    if (!do_request(req, resp)) return "Daemon not running";
    if (resp.value("type","") == "ERROR")
        return resp.value("message", "Unknown error");
    return {};
}

void IpcClient::startSubscription() {
    if (subscribed_) return;
    subscribed_ = true;
    tryConnect();
    reconnect_timer_->start();
}

void IpcClient::stopSubscription() {
    subscribed_ = false;
    reconnect_timer_->stop();
    sub_socket_->disconnectFromServer();
}

void IpcClient::tryConnect() {
    if (sub_socket_->state() != QLocalSocket::UnconnectedState) return;

    sub_socket_->connectToServer(SOCKET_PATH);
    if (!sub_socket_->waitForConnected(500)) {
        emit connectionChanged(false);
        return;
    }

    // Send SUBSCRIBE command
    std::string body = R"({"cmd":"SUBSCRIBE"})";
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {uint8_t(len),uint8_t(len>>8),uint8_t(len>>16),uint8_t(len>>24)};
    sub_socket_->write(reinterpret_cast<const char*>(hdr), 4);
    sub_socket_->write(body.data(), body.size());
    sub_socket_->flush();

    emit connectionChanged(true);
}

void IpcClient::onSubDataReady() {
    sub_buf_ += sub_socket_->readAll();

    while (sub_buf_.size() >= 4) {
        uint32_t len = static_cast<uint8_t>(sub_buf_[0])
                     | (static_cast<uint8_t>(sub_buf_[1]) << 8)
                     | (static_cast<uint8_t>(sub_buf_[2]) << 16)
                     | (static_cast<uint8_t>(sub_buf_[3]) << 24);

        if (static_cast<uint32_t>(sub_buf_.size()) < 4 + len) break;

        QByteArray body = sub_buf_.mid(4, len);
        sub_buf_.remove(0, 4 + len);

        try {
            auto msg = nlohmann::json::parse(body.constData(), body.constData() + body.size());
            if (msg.value("type","") == "EVENT") {
                emit mouseEventReceived(
                    msg.value("x", 0.0),
                    msg.value("y", 0.0),
                    msg.value("time_ms", 1.0)
                );
            }
        } catch (...) {}
    }
}

void IpcClient::onSubDisconnected() {
    sub_buf_.clear();
    emit connectionChanged(false);
    // Reconnect quickly rather than waiting for the next timer tick (up to 5 s).
    if (subscribed_)
        QTimer::singleShot(300, this, &IpcClient::tryConnect);
}
