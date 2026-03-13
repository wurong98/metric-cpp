#include "metric/calculator/metrics_calculator.h"
#include <cmath>

namespace metric {

MetricsCalculator::MetricsCalculator(int ofi_window, int depth_levels, double smoothing)
    : ofi_window_(ofi_window)
    , depth_levels_(depth_levels)
    , smoothing_(smoothing)
    , ema_microprice_(0.0)
    , initialized_(false)
{}

Metrics MetricsCalculator::calculate(const OrderBook& book, const std::deque<Trade>& trades) {
    Metrics m;
    m.mid_price = book.get_mid_price();
    m.bid_depth = book.get_bid_depth(depth_levels_);
    m.ask_depth = book.get_ask_depth(depth_levels_);

    // Microprice: 价格加权计算
    // Microprice = (sum(Q_bid * P_ask) + sum(Q_ask * P_bid)) / (sum(Q_bid) + sum(Q_ask))
    double weighted_bid = book.get_weighted_bid_depth(depth_levels_);
    double weighted_ask = book.get_weighted_ask_depth(depth_levels_);
    double total_depth = m.bid_depth + m.ask_depth;
    double raw_microprice = 0.0;
    if (total_depth > 0) {
        raw_microprice = (weighted_bid + weighted_ask) / total_depth;
    } else {
        raw_microprice = m.mid_price;
    }

    // EMA smoothing
    if (!initialized_) {
        ema_microprice_ = raw_microprice;
        initialized_ = true;
    } else {
        ema_microprice_ = smoothing_ * raw_microprice + (1 - smoothing_) * ema_microprice_;
    }
    m.microprice = ema_microprice_;

    // Depth Imbalance
    if (m.bid_depth + m.ask_depth > 0) {
        m.depth_imbalance = (m.bid_depth - m.ask_depth) / (m.bid_depth + m.ask_depth);
    } else {
        m.depth_imbalance = 0.0;
    }

    // OFI
    double ofi = 0.0;
    int count = 0;
    for (auto it = trades.rbegin(); it != trades.rend() && count < ofi_window_; ++it, ++count) {
        if (it->is_buy) {
            ofi += it->size;
        } else {
            ofi -= it->size;
        }
    }
    m.ofi = ofi;

    return m;
}

} // namespace metric
