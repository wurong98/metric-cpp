#include "metric/models/order_book.h"
#include <cassert>

namespace metric {

void OrderBook::clear() {
    bids.clear();
    asks.clear();
}

double OrderBook::get_mid_price() const {
    if (bids.empty() || asks.empty()) return 0.0;
    return (bids.front().price + asks.front().price) / 2.0;
}

double OrderBook::get_spread() const {
    if (bids.empty() || asks.empty()) return 0.0;
    return asks.front().price - bids.front().price;
}

double OrderBook::get_bid_depth(int levels) const {
    double depth = 0.0;
    for (int i = 0; i < levels && i < static_cast<int>(bids.size()); ++i) {
        depth += bids[i].size;
    }
    return depth;
}

double OrderBook::get_ask_depth(int levels) const {
    double depth = 0.0;
    for (int i = 0; i < levels && i < static_cast<int>(asks.size()); ++i) {
        depth += asks[i].size;
    }
    return depth;
}

double OrderBook::get_weighted_bid_depth(int levels) const {
    // 加权深度: 买单数量用对应档位卖价加权 (Q_bid * P_ask)
    // 例如: 第1档买单 * 第1档卖价 + 第2档买单 * 第2档卖价 + ...
    double weighted_depth = 0.0;
    int num_levels = std::min(levels, std::min(static_cast<int>(bids.size()), static_cast<int>(asks.size())));
    for (int i = 0; i < num_levels; ++i) {
        weighted_depth += bids[i].size * asks[i].price;
    }
    return weighted_depth;
}

double OrderBook::get_weighted_ask_depth(int levels) const {
    // 加权深度: 卖单数量用对应档位买价加权 (Q_ask * P_bid)
    // 例如: 第1档卖单 * 第1档买价 + 第2档卖单 * 第2档买价 + ...
    double weighted_depth = 0.0;
    int num_levels = std::min(levels, std::min(static_cast<int>(bids.size()), static_cast<int>(asks.size())));
    for (int i = 0; i < num_levels; ++i) {
        weighted_depth += asks[i].size * bids[i].price;
    }
    return weighted_depth;
}

} // namespace metric
