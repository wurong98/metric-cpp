// src/main.cpp
#include <iostream>
#include <cstring>
#include <signal.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "metric/config.h"
#include "metric/calculator/metrics_calculator.h"
#include "metric/websocket/client.h"
#include "metric/models/order_book.h"
#include <nlohmann/json.hpp>

using namespace metric;

// Global flag for signal handling
static volatile bool g_running = true;

std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void signal_handler(int) {
    g_running = false;
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [config_path] [-r rate]" << std::endl;
    std::cout << "  config_path: Path to config file (default: config/config.yaml)" << std::endl;
    std::cout << "  -r rate:     Output rate (0=full, 1=1Hz, 10=10Hz, default: 0)" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse arguments
    std::string config_path = "config/config.yaml";
    int rate = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
            rate = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            config_path = argv[i];
        }
    }

    // Load config
    Config config = load_config(config_path);
    std::cout << "Loaded config from " << config_path << std::endl;
    std::cout << "WebSocket URL: " << config.websocket.url << std::endl;
    std::cout << "Rate: " << (rate == 0 ? "full" : std::to_string(rate) + "Hz") << std::endl;

    // Create calculator
    MetricsCalculator calc(
        config.calculator.ofi_window,
        config.calculator.depth_levels,
        config.calculator.smoothing
    );

    // Create WebSocket client
    WebSocketClient ws(config);
    ws.set_rate(rate);

    // Metrics callback
    ws.set_metrics_callback([](const Metrics& m) {
        printf("%s | Microprice: %.4f | DepthImbalance: %.4f | OFI: %.4f\n",
               get_timestamp().c_str(), m.microprice, m.depth_imbalance, m.ofi);
        fflush(stdout);
    });

    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Connect and run
    if (!ws.connect()) {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }

    std::cout << "Starting WebSocket client... (Press Ctrl+C to stop)" << std::endl;

    // Run in a separate thread to allow signal handling
    std::thread ws_thread([&ws]() {
        ws.run();
    });

    // Wait for signal
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down..." << std::endl;
    ws.disconnect();

    if (ws_thread.joinable()) {
        ws_thread.join();
    }

    std::cout << "Done." << std::endl;
    return 0;
}
