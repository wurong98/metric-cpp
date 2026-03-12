// src/main.cpp
#include <iostream>
#include "metric/config.h"
#include "metric/calculator/metrics_calculator.h"
#include "metric/websocket/client.h"
#include "metric/models/order_book.h"
#include <nlohmann/json.hpp>

using namespace metric;

int main(int argc, char* argv[]) {
    // Load config
    std::string config_path = "config/config.yaml";
    if (argc > 1) {
        config_path = argv[1];
    }

    Config config = load_config(config_path);
    std::cout << "Loaded config from " << config_path << std::endl;
    std::cout << "WebSocket URL: " << config.websocket.url << std::endl;

    // Create calculator
    MetricsCalculator calc(
        config.calculator.ofi_window,
        config.calculator.depth_levels,
        config.calculator.smoothing
    );

    // Create WebSocket client
    WebSocketClient ws(config);
    ws.set_message_callback([&calc](const std::string& msg) {
        try {
            auto j = nlohmann::json::parse(msg);
            // Parse orderbook and calculate metrics
            // (simplified for now)
        } catch (const std::exception& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
        }
    });

    ws.set_metrics_callback([](const Metrics& m) {
        printf("Microprice: %.4f | DepthImbalance: %.4f | OFI: %.4f\n",
               m.microprice, m.depth_imbalance, m.ofi);
    });

    std::cout << "Starting WebSocket client..." << std::endl;

    // For now, just run with dummy data
    OrderBook book;
    book.bids.push_back({50000.0, 1.5});
    book.bids.push_back({49999.0, 2.0});
    book.asks.push_back({50001.0, 1.2});
    book.asks.push_back({50002.0, 3.0});

    Metrics m = calc.calculate(book, {});
    printf("Microprice: %.4f | DepthImbalance: %.4f | OFI: %.4f\n",
           m.microprice, m.depth_imbalance, m.ofi);

    return 0;
}
