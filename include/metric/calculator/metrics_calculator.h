#pragma once
#include "metric/models/order_book.h"
#include <cstdint>
#include <deque>

namespace metric {

struct Trade {
    double price;
    double size;
    bool is_buy;  // true = buy, false = sell
    uint64_t timestamp;
};

struct Metrics {
    double microprice;
    double depth_imbalance;
    double ofi;
    double mid_price;
    double bid_depth;
    double ask_depth;
};

class MetricsCalculator {
public:
    explicit MetricsCalculator(int ofi_window, int depth_levels, double smoothing);

    Metrics calculate(const OrderBook& book, const std::deque<Trade>& trades);

private:
    int ofi_window_;
    int depth_levels_;
    double smoothing_;
    double ema_microprice_;
    bool initialized_;
};

} // namespace metric
