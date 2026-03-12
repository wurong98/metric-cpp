#include "metric/websocket/client.h"
#include <libwebsockets.h>
#include <cstring>
#include <iostream>
#include <nlohmann/json.hpp>

namespace metric {

struct WebSocketClient::Impl {
    const Config& config;
    struct lws_context* context = nullptr;
    struct lws* wsi = nullptr;
    MessageCallback msg_cb;
    MetricsCallback metrics_cb;
    bool connected = false;

    Impl(const Config& c) : config(c) {}
};

WebSocketClient::WebSocketClient(const Config& config)
    : impl_(std::make_unique<Impl>(config))
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

bool WebSocketClient::connect() {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = nullptr;
    info.iface = nullptr;
    info.ssl_cert_filepath = nullptr;
    info.ssl_private_key_filepath = nullptr;
    info.extensions = nullptr;
    info.gid = -1;
    info.uid = -1;
    info.options = LWS_SERVER_OPTION_VALIDATE_UTF8;

    impl_->context = lws_create_context(&info);
    if (!impl_->context) {
        std::cerr << "Failed to create lws context" << std::endl;
        return false;
    }

    // Connect will be done in run()
    return true;
}

void WebSocketClient::disconnect() {
    if (impl_->wsi) {
        lws_close_reason(impl_->wsi, LWS_CLOSE_STATUS_NORMAL, nullptr, 0);
        impl_->wsi = nullptr;
    }
    if (impl_->context) {
        lws_context_destroy(impl_->context);
        impl_->context = nullptr;
    }
}

void WebSocketClient::run() {
    // Simplified: just wait for connection in this version
    // Full implementation would handle connection loop
    while (impl_->connected || true) {
        lws_service(impl_->context, 100);
    }
}

void WebSocketClient::subscribe_orderbook() {
    // To be implemented with Hyperliquid API
}

void WebSocketClient::subscribe_trades() {
    // To be implemented with Hyperliquid API
}

void WebSocketClient::handle_message(const std::string& msg) {
    if (impl_->msg_cb) {
        impl_->msg_cb(msg);
    }
}

} // namespace metric
