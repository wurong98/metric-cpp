#pragma once
#include "metric/config.h"
#include "metric/models/order_book.h"
#include "metric/calculator/metrics_calculator.h"
#include <functional>
#include <string>
#include <memory>

// Forward declaration
struct lws;

namespace metric {

class WebSocketClient {
public:
    using MessageCallback = std::function<void(const std::string& msg)>;
    using MetricsCallback = std::function<void(const Metrics& m)>;

    WebSocketClient(const Config& config);
    ~WebSocketClient();

    void set_message_callback(MessageCallback cb);
    void set_metrics_callback(MetricsCallback cb);

    bool connect();
    void disconnect();
    void run();

private:
    void handle_message(const std::string& msg);
    void subscribe_orderbook();
    void subscribe_trades();

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace metric
