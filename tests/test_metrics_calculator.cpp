#include <gtest/gtest.h>
#include "metric/calculator/metrics_calculator.h"

using namespace metric;

TEST(MetricsCalculatorTest, Basic) {
    MetricsCalculator calc(10, 10, 0.1);
    OrderBook book;
    book.bids.push_back({100.0, 5.0});
    book.asks.push_back({101.0, 5.0});

    Metrics m = calc.calculate(book, {});

    EXPECT_DOUBLE_EQ(m.mid_price, 100.5);
    EXPECT_DOUBLE_EQ(m.depth_imbalance, 0.0);
}

TEST(MetricsCalculatorTest, DepthImbalance) {
    MetricsCalculator calc(10, 10, 0.1);
    OrderBook book;
    book.bids.push_back({100.0, 10.0});
    book.asks.push_back({101.0, 5.0});

    Metrics m = calc.calculate(book, {});

    EXPECT_DOUBLE_EQ(m.depth_imbalance, 1.0/3.0);  // (10-5)/(10+5)
}
