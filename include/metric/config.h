#pragma once
#include <string>

namespace metric {

struct Config {
    struct WebSocket {
        std::string url;
        int ping_interval;
        int reconnect_delay;
        int max_reconnect_attempts;
    } websocket;

    struct Symbol {
        std::string name;
        std::string pair;
    } symbol;

    struct Calculator {
        int ofi_window;
        int depth_levels;
        double smoothing;
    } calculator;

    std::string log_level;
};

Config load_config(const std::string& path);

} // namespace metric
