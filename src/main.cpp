// src/main.cpp
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include "metric/config.h"
#include "metric/calculator/metrics_calculator.h"
#include "metric/websocket/client.h"
#include "metric/models/order_book.h"
#include <nlohmann/json.hpp>

using namespace metric;

static std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

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

    // Current order book and trades
    OrderBook book;
    std::deque<Trade> trades;

    // Setup signal handler for graceful exit
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "Starting metrics calculation... (Press Ctrl+C to exit)" << std::endl;

    // Initialize with dummy data for initial output
    book.bids.push_back({50000.0, 1.5});
    book.bids.push_back({49999.0, 2.0});
    book.asks.push_back({50001.0, 1.2});
    book.asks.push_back({50002.0, 3.0});

    // Main loop: calculate and print metrics every second
    while (g_running) {
        Metrics m = calc.calculate(book, trades);
        printf("Microprice: %.4f | DepthImbalance: %.4f | OFI: %.4f\n",
               m.microprice, m.depth_imbalance, m.ofi);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Shutting down..." << std::endl;
    return 0;
}
