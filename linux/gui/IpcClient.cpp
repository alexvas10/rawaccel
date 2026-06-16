#include "IpcClient.hpp"
#include <QDataStream>

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
    QLocalSocket sock;
    sock.connectToServer(SOCKET_PATH);
    if (!sock.waitForConnected(2000)) return false;

    std::string body = req.dump();
    uint32_t len = static_cast<uint32_t>(body.size());
    uint8_t hdr[4] = {
        uint8_t(len & 0xFF), uint8_t((len>>8)&0xFF),
        uint8_t((len>>16)&0xFF), uint8_t((len>>24)&0xFF)
    };
    sock.write(reinterpret_cast<const char*>(hdr), 4);
    sock.write(body.data(), body.size());
    sock.flush();

    // Read response header
    if (!sock.waitForReadyRead(3000)) return false;
    QByteArray rsp_data;
    while (rsp_data.size() < 4 && sock.waitForReadyRead(1000))
        rsp_data += sock.readAll();
    if (rsp_data.size() < 4) return false;

    uint32_t rlen = static_cast<uint8_t>(rsp_data[0])
                  | (static_cast<uint8_t>(rsp_data[1]) << 8)
                  | (static_cast<uint8_t>(rsp_data[2]) << 16)
                  | (static_cast<uint8_t>(rsp_data[3]) << 24);

    QByteArray body_data = rsp_data.mid(4);
    while (static_cast<uint32_t>(body_data.size()) < rlen && sock.waitForReadyRead(1000))
        body_data += sock.readAll();

    try {
        resp = nlohmann::json::parse(body_data.constData(), body_data.constData() + body_data.size());
        return true;
    } catch (...) {
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
}
