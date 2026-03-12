#include <gtest/gtest.h>
#include "metric/models/order_book.h"

using namespace metric;

TEST(OrderBookTest, MidPrice) {
    OrderBook ob;
    ob.bids.push_back({100.0, 1.0});
    ob.asks.push_back({101.0, 1.0});
    EXPECT_DOUBLE_EQ(ob.get_mid_price(), 100.5);
}

TEST(OrderBookTest, Spread) {
    OrderBook ob;
    ob.bids.push_back({100.0, 1.0});
    ob.asks.push_back({101.0, 1.0});
    EXPECT_DOUBLE_EQ(ob.get_spread(), 1.0);
}

TEST(OrderBookTest, Depth) {
    OrderBook ob;
    ob.bids.push_back({100.0, 1.0});
    ob.bids.push_back({99.0, 2.0});
    ob.asks.push_back({101.0, 1.5});
    ob.asks.push_back({102.0, 2.5});

    EXPECT_DOUBLE_EQ(ob.get_bid_depth(2), 3.0);
    EXPECT_DOUBLE_EQ(ob.get_ask_depth(2), 4.0);
}
