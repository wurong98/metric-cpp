#pragma once
#include <vector>
#include <cstddef>

namespace metric {

struct OrderLevel {
    double price;
    double size;
};

struct OrderBook {
    std::vector<OrderLevel> bids;  // 买单 (价格升序)
    std::vector<OrderLevel> asks; // 卖单 (价格升序)

    void clear();
    double get_mid_price() const;
    double get_spread() const;
    double get_bid_depth(int levels) const;
    double get_ask_depth(int levels) const;
    double get_weighted_bid_depth(int levels) const;  // 价格加权深度 (用于Microprice)
    double get_weighted_ask_depth(int levels) const;
};

} // namespace metric
