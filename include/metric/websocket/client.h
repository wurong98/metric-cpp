#pragma once
#include "metric/config.h"
#include "metric/models/order_book.h"
#include "metric/calculator/metrics_calculator.h"
#include <libwebsockets.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>

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

    // Rate control
    void set_rate(int rate); // 0=full, 1=1Hz, 10=10Hz
    int get_rate() const;

    // For callback access
    void handle_message(const std::string& msg);
    void subscribe_orderbook();
    void subscribe_trades();

    // Allow callback to access impl
    struct Impl;
    std::unique_ptr<Impl> impl_;

private:
    void parse_orderbook(const nlohmann::json& j);
    void parse_trade(const nlohmann::json& j);
    void output_metrics();

    int rate_ = 0; // 0=full, 1=1Hz, 10=10Hz
    std::atomic<bool> running_{false};
    std::thread worker_thread_;
    std::mutex metrics_mutex_;
    Metrics latest_metrics_;
    std::atomic<int> output_count_{0};

    // For OFI calculation
    std::deque<Trade> trades_;
    std::mutex trades_mutex_;
    int ofi_window_ = 10;
    int depth_levels_ = 10;
    double smoothing_ = 0.1;
    double ema_microprice_ = 0.0;
    bool ema_initialized_ = false;

    void calculate_metrics(const OrderBook& book);
};

} // namespace metric
