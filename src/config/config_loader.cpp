#include "metric/config.h"
#include <yaml-cpp/yaml.h>

namespace metric {

Config load_config(const std::string& path) {
    YAML::Node config = YAML::LoadFile(path);
    Config cfg;

    cfg.websocket.url = config["websocket"]["url"].as<std::string>();
    cfg.websocket.ping_interval = config["websocket"]["ping_interval"].as<int>(30);
    cfg.websocket.reconnect_delay = config["websocket"]["reconnect_delay"].as<int>(1);
    cfg.websocket.max_reconnect_attempts = config["websocket"]["max_reconnect_attempts"].as<int>(10);

    cfg.symbol.name = config["symbol"]["name"].as<std::string>("BTC");
    cfg.symbol.pair = config["symbol"]["pair"].as<std::string>("BTC-USD-PERP");

    cfg.calculator.ofi_window = config["calculator"]["ofi_window"].as<int>(10);
    cfg.calculator.depth_levels = config["calculator"]["depth_levels"].as<int>(10);
    cfg.calculator.smoothing = config["calculator"]["smoothing"].as<double>(0.1);

    cfg.log_level = config["log_level"].as<std::string>("info");

    return cfg;
}

} // namespace metric
