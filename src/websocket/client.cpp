#include "metric/websocket/client.h"
#include <libwebsockets.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

namespace metric {

struct WebSocketClient::Impl {
    const Config& config;
    struct lws_context* context = nullptr;
    struct lws* wsi = nullptr;
    MessageCallback msg_cb;
    MetricsCallback metrics_cb;
    bool connected = false;
    bool subscribed = false;
    std::string pending_message;

    Impl(const Config& c) : config(c) {}
};

static int callback_ws(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len);

static struct lws_protocols protocols[] = {
    {
        "hyperliquid",
        callback_ws,
        0,
        65535,
    },
    { NULL, NULL, 0, 0 }
};

WebSocketClient::WebSocketClient(const Config& config)
    : impl_(std::make_unique<Impl>(config))
    , ofi_window_(config.calculator.ofi_window)
    , depth_levels_(config.calculator.depth_levels)
    , smoothing_(config.calculator.smoothing)
{}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::set_message_callback(MessageCallback cb) {
    impl_->msg_cb = std::move(cb);
}

void WebSocketClient::set_metrics_callback(MetricsCallback cb) {
    impl_->metrics_cb = std::move(cb);
}

void WebSocketClient::set_rate(int rate) {
    rate_ = rate;
}

int WebSocketClient::get_rate() const {
    return rate_;
}

bool WebSocketClient::connect() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.iface = nullptr;
    info.ssl_cert_filepath = nullptr;
    info.ssl_private_key_filepath = nullptr;
    info.extensions = nullptr;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
                   LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.user = this;

    impl_->context = lws_create_context(&info);
    if (!impl_->context) {
        std::cerr << "Failed to create lws context" << std::endl;
        return false;
    }

    std::cout << "Context created, user=" << this << std::endl;

    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = impl_->context;
    ccinfo.address = "api.hyperliquid.xyz";
    ccinfo.port = 443;
    ccinfo.path = "/ws";
    ccinfo.host = "api.hyperliquid.xyz";
    ccinfo.origin = "https://hyperliquid.xyz";
    ccinfo.ssl_connection = 1;
    ccinfo.pwsi = &impl_->wsi;

    impl_->wsi = lws_client_connect_via_info(&ccinfo);
    if (!impl_->wsi) {
        std::cerr << "Failed to initiate connection" << std::endl;
        lws_context_destroy(impl_->context);
        impl_->context = nullptr;
        return false;
    }

    std::cout << "Connecting to wss://api.hyperliquid.xyz/ws..." << std::endl;
    return true;
}

void WebSocketClient::disconnect() {
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    if (impl_->wsi) {
        impl_->wsi = nullptr;
    }
    if (impl_->context) {
        lws_context_destroy(impl_->context);
        impl_->context = nullptr;
    }
}

void WebSocketClient::run() {
    running_ = true;

    // Start output worker thread
    worker_thread_ = std::thread([this]() {
        while (running_) {
            if (rate_ > 0) {
                int interval_ms = 1000 / rate_;
                std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            output_metrics();
        }
    });

    // Main event loop - wait for connection
    while (running_) {
        if (impl_->context) {
            lws_service(impl_->context, 100);
        } else {
            break;
        }
    }
}

void WebSocketClient::output_metrics() {
    if (!impl_->metrics_cb) return;

    std::lock_guard<std::mutex> lock(metrics_mutex_);
    if (output_count_.load() > 0 || rate_ == 0) {
        impl_->metrics_cb(latest_metrics_);
        if (rate_ > 0) {
            output_count_.fetch_sub(1);
        }
    }
}

void WebSocketClient::subscribe_orderbook() {
    if (!impl_->wsi) return;

    // Use correct Hyperliquid API format
    nlohmann::json sub = {
        {"method", "subscribe"},
        {"subscription", {
            {"type", "l2Book"},
            {"coin", impl_->config.symbol.name}
        }}
    };

    std::string msg = sub.dump();
    std::cout << "Sending: " << msg << std::endl;
    unsigned char buf[LWS_PRE + 4096];
    memcpy(buf + LWS_PRE, msg.c_str(), msg.size());

    lws_write(impl_->wsi, buf + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
    std::cout << "Subscribed to l2Book" << std::endl;
}

void WebSocketClient::subscribe_trades() {
    if (!impl_->wsi) return;

    nlohmann::json sub = {
        {"method", "subscribe"},
        {"subscription", {
            {"type", "trades"},
            {"coin", impl_->config.symbol.name}
        }}
    };

    std::string msg = sub.dump();
    std::cout << "Sending: " << msg << std::endl;
    unsigned char buf[LWS_PRE + 4096];
    memcpy(buf + LWS_PRE, msg.c_str(), msg.size());

    lws_write(impl_->wsi, buf + LWS_PRE, msg.size(), LWS_WRITE_TEXT);
    std::cout << "Subscribed to trades" << std::endl;
}

static int callback_ws(struct lws* wsi, enum lws_callback_reasons reason,
                       void* user, void* in, size_t len) {
    // Get WebSocketClient instance from context user data
    struct lws_context* ctx = lws_get_context(wsi);
    WebSocketClient* client = (WebSocketClient*)lws_context_user(ctx);

    if (!client) {
        return 0;
    }

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            std::cout << "WebSocket connected!" << std::endl;
            fflush(stdout);
            client->impl_->connected = true;
            if (!client->impl_->subscribed) {
                client->subscribe_orderbook();
                client->subscribe_trades();
                client->impl_->subscribed = true;
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            std::string msg((const char*)in, len);
            client->handle_message(msg);
            break;
        }

        case LWS_CALLBACK_CLIENT_CLOSED:
            std::cout << "WebSocket closed" << std::endl;
            fflush(stdout);
            client->impl_->connected = false;
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "Connection error" << std::endl;
            fflush(stdout);
            client->impl_->connected = false;
            break;

        default:
            break;
    }
    return 0;
}

void WebSocketClient::handle_message(const std::string& msg) {
    if (impl_->msg_cb) {
        impl_->msg_cb(msg);
    }

    try {
        auto j = nlohmann::json::parse(msg);

        // Check for channel (Hyperliquid uses "channel" field)
        if (j.contains("channel")) {
            std::string channel = j["channel"];

            if (channel == "l2Book") {
                parse_orderbook(j["data"]);
            } else if (channel == "trades") {
                parse_trade(j["data"]);
            } else if (channel == "subscriptionResponse") {
                // Subscription confirmed silently
            } else {
                std::cout << "DEBUG: Unknown channel: " << channel << std::endl;
            }
        } else if (j.contains("type") && j["type"] == "error") {
            std::cerr << "Error: " << j.dump() << std::endl;
        } else {
            // Print first 200 chars of unknown message for debugging
            std::cout << "DEBUG: Unknown message: " << msg.substr(0, 300) << std::endl;
        }
    } catch (const std::exception& e) {
        // Ignore parse errors for now
    }
}

void WebSocketClient::parse_orderbook(const nlohmann::json& j) {
    try {
        OrderBook book;

        // Hyperliquid format: { "coin": "BTC", "levels": [[bids], [asks]], "time": 123 }
        // Each level: { "px": "50000", "sz": "1.5", "n": 10 }

        // Parse levels array - [bids, asks]
        if (j.contains("levels") && j["levels"].is_array() && j["levels"].size() == 2) {
            auto& bids = j["levels"][0];
            auto& asks = j["levels"][1];

            // Parse bids
            if (bids.is_array()) {
                for (const auto& bid : bids) {
                    if (bid.contains("px") && bid.contains("sz")) {
                        double price = std::stod(bid["px"].get<std::string>());
                        double size = std::stod(bid["sz"].get<std::string>());
                        book.bids.push_back({price, size});
                    }
                }
            }

            // Parse asks
            if (asks.is_array()) {
                for (const auto& ask : asks) {
                    if (ask.contains("px") && ask.contains("sz")) {
                        double price = std::stod(ask["px"].get<std::string>());
                        double size = std::stod(ask["sz"].get<std::string>());
                        book.asks.push_back({price, size});
                    }
                }
            }
        }

        // Calculate metrics
        {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            if (!book.bids.empty() && !book.asks.empty()) {
                double best_bid = book.bids[0].price;
                double best_ask = book.asks[0].price;
                double mid_price = (best_bid + best_ask) / 2.0;
                double spread = best_ask - best_bid;

                double bid_depth = 0, ask_depth = 0;
                int levels = std::min((int)book.bids.size(), depth_levels_);
                for (int i = 0; i < levels; i++) {
                    bid_depth += book.bids[i].size;
                    ask_depth += book.asks[i].size;
                }

                double imbalance = (bid_depth - ask_depth) / (bid_depth + ask_depth + 1e-10);

                // Microprice with EMA smoothing
                double raw_microprice = mid_price + imbalance * spread / 2.0;
                if (!ema_initialized_) {
                    ema_microprice_ = raw_microprice;
                    ema_initialized_ = true;
                } else {
                    ema_microprice_ = smoothing_ * raw_microprice + (1 - smoothing_) * ema_microprice_;
                }

                latest_metrics_.microprice = ema_microprice_;
                latest_metrics_.depth_imbalance = imbalance;
                latest_metrics_.bid_depth = bid_depth;
                latest_metrics_.ask_depth = ask_depth;
                latest_metrics_.mid_price = mid_price;

                // Calculate OFI from trades
                double ofi = 0.0;
                {
                    std::lock_guard<std::mutex> trades_lock(trades_mutex_);
                    int count = 0;
                    for (auto it = trades_.rbegin(); it != trades_.rend() && count < ofi_window_; ++it, ++count) {
                        if (it->is_buy) {
                            ofi += it->size;
                        } else {
                            ofi -= it->size;
                        }
                    }
                }
                latest_metrics_.ofi = ofi;
            }
        }

        if (rate_ > 0) {
            output_count_.fetch_add(1);
        }

    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
    }
}

void WebSocketClient::parse_trade(const nlohmann::json& j) {
    try {
        // Hyperliquid trades format: array of trade objects
        // Each trade: { "px": "50000", "sz": "0.1", "side": "A", "tid": ..., "time": ... }
        // px=price, sz=size, side="A"(Ask/Sell) or "B"(Bid/Buy)

        if (!j.is_array()) return;

        std::lock_guard<std::mutex> lock(trades_mutex_);

        for (const auto& trade : j) {
            if (!trade.contains("px") || !trade.contains("sz") || !trade.contains("side")) {
                continue;
            }

            Trade t;
            t.price = std::stod(trade["px"].get<std::string>());
            t.size = std::stod(trade["sz"].get<std::string>());
            // side: "B" = Bid/Buy, "A" = Ask/Sell
            std::string side = trade["side"].get<std::string>();
            t.is_buy = (side == "B");
            t.timestamp = trade.contains("time") ? trade["time"].get<uint64_t>() : 0;

            trades_.push_back(t);
            // std::cout << "DEBUG: Added trade: side=" << side << ", is_buy=" << t.is_buy << ", sz=" << t.size << std::endl;

            // Keep only recent trades within OFI window
            if ((int)trades_.size() > ofi_window_ * 2) {
                trades_.pop_front();
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Trade parse error: " << e.what() << std::endl;
    }
}

} // namespace metric
