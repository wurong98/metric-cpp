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

} // namespace metric
