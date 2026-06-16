#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QTimer>
#include <nlohmann/json.hpp>
#include <string>

// Qt-based Unix socket client to the rawacceld daemon.
// Equivalent to the C# DriverConfig.GetActive() / Activate() / events.
class IpcClient : public QObject {
    Q_OBJECT
public:
    explicit IpcClient(QObject* parent = nullptr);
    ~IpcClient() override;

    bool isConnected() const;

    // Synchronous: send READ, return settings JSON (empty on failure).
    nlohmann::json readSettings();

    // Synchronous: send WRITE with settings JSON.
    // Returns error string or empty string on success.
    std::string writeSettings(const nlohmann::json& settings);

    // Start streaming mouse events (SUBSCRIBE).
    void startSubscription();
    void stopSubscription();

signals:
    void mouseEventReceived(double x, double y, double time_ms);
    void connectionChanged(bool connected);

private slots:
    void tryConnect();
    void onSubDataReady();
    void onSubDisconnected();

private:
    QLocalSocket* sub_socket_ = nullptr;
    QTimer*       reconnect_timer_;
    bool          subscribed_ = false;
    QByteArray    sub_buf_;

    bool do_request(const nlohmann::json& req, nlohmann::json& resp);
    static constexpr const char* SOCKET_PATH = "/var/run/rawaccel.sock";
};
