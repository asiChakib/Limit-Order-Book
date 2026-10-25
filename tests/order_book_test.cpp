
#include "order_book.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

class CoutCapture {
public:
    CoutCapture() : old_buffer_(std::cout.rdbuf(buffer_.rdbuf())) {}

    ~CoutCapture() {
        std::cout.rdbuf(old_buffer_);
    }

    CoutCapture(const CoutCapture&) = delete;
    CoutCapture& operator=(const CoutCapture&) = delete;

    [[nodiscard]] std::string str() const {
        return buffer_.str();
    }

private:
    std::ostringstream buffer_;
    std::streambuf* old_buffer_;
};

TEST(OrderBookTest, NonCrossingOrdersRemainInBook) {
    OrderBook book;

    {
        CoutCapture capture;
        book.submit({1, Side::Sell, 101, 5});
        book.submit({2, Side::Buy, 100, 8});
    }

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_NE(output.find("Price 101 | Total quantity 5"), std::string::npos);
    EXPECT_NE(output.find("[1: 5]"), std::string::npos);
    EXPECT_NE(output.find("Price 100 | Total quantity 8"), std::string::npos);
    EXPECT_NE(output.find("[2: 8]"), std::string::npos);
}

TEST(OrderBookTest, ExactFillRemovesRestingOrder) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 5});

    std::string trades;
    {
        CoutCapture capture;
        book.submit({2, Side::Buy, 101, 5});
        trades = capture.str();
    }

    EXPECT_NE(trades.find("buy order 2 vs sell order 1"), std::string::npos);
    EXPECT_NE(trades.find("quantity of 5"), std::string::npos);

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_EQ(output.find("Price 101"), std::string::npos);
    EXPECT_EQ(output.find("[1:"), std::string::npos);
}

TEST(OrderBookTest, PartialFillLeavesCorrectRemainingQuantity) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 10});

    {
        CoutCapture capture;
        book.submit({2, Side::Buy, 101, 4});
    }

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_NE(output.find("Price 101 | Total quantity 6"), std::string::npos);
    EXPECT_NE(output.find("[1: 6]"), std::string::npos);
    EXPECT_EQ(output.find("[2:"), std::string::npos);
}

TEST(OrderBookTest, BestPriceHasPriority) {
    OrderBook book;
    book.submit({1, Side::Sell, 102, 3});
    book.submit({2, Side::Sell, 101, 3});

    std::string trades;
    {
        CoutCapture capture;
        book.submit({3, Side::Buy, 102, 4});
        trades = capture.str();
    }

    const auto best_price_trade = trades.find("sell order 2 at price 101");
    const auto next_price_trade = trades.find("sell order 1 at price 102");

    ASSERT_NE(best_price_trade, std::string::npos);
    ASSERT_NE(next_price_trade, std::string::npos);
    EXPECT_LT(best_price_trade, next_price_trade);
}

TEST(OrderBookTest, FIFOIsPreservedAtSamePrice) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 3});
    book.submit({2, Side::Sell, 101, 4});

    std::string trades;
    {
        CoutCapture capture;
        book.submit({3, Side::Buy, 101, 5});
        trades = capture.str();
    }

    const auto first_order_trade = trades.find("sell order 1");
    const auto second_order_trade = trades.find("sell order 2");

    ASSERT_NE(first_order_trade, std::string::npos);
    ASSERT_NE(second_order_trade, std::string::npos);
    EXPECT_LT(first_order_trade, second_order_trade);

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_NE(output.find("[2: 2]"), std::string::npos);
    EXPECT_EQ(output.find("[1:"), std::string::npos);
}

TEST(OrderBookTest, IncomingOrderCanMatchMultiplePriceLevels) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 2});
    book.submit({2, Side::Sell, 102, 3});

    std::string trades;
    {
        CoutCapture capture;
        book.submit({3, Side::Buy, 102, 5});
        trades = capture.str();
    }

    EXPECT_NE(trades.find("sell order 1 at price 101"), std::string::npos);
    EXPECT_NE(trades.find("sell order 2 at price 102"), std::string::npos);

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_EQ(output.find("Price 101"), std::string::npos);
    EXPECT_EQ(output.find("Price 102"), std::string::npos);
}

TEST(OrderBookTest, CancellationRemovesOrder) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 5});
    book.submit({2, Side::Sell, 101, 7});

    EXPECT_TRUE(book.cancel(1));
    EXPECT_FALSE(book.cancel(1));

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_NE(output.find("Price 101 | Total quantity 7"), std::string::npos);
    EXPECT_EQ(output.find("[1:"), std::string::npos);
    EXPECT_NE(output.find("[2: 7]"), std::string::npos);
}

TEST(OrderBookTest, CancellingUnknownOrderReturnsFalse) {
    OrderBook book;

    EXPECT_FALSE(book.cancel(999));
}

TEST(OrderBookTest, CancellingLastOrderRemovesPriceLevel) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 5});
    book.submit({2, Side::Sell, 102, 6});

    EXPECT_TRUE(book.cancel(1));

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_EQ(output.find("Price 101"), std::string::npos);
    EXPECT_NE(output.find("Price 102 | Total quantity 6"), std::string::npos);
}

TEST(OrderBookTest, DuplicateRestingOrderIdIsRejected) {
    OrderBook book;
    book.submit({1, Side::Sell, 101, 5});

    EXPECT_THROW(book.submit({1, Side::Buy, 100, 2}), std::invalid_argument);

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_NE(output.find("Price 101 | Total quantity 5"), std::string::npos);
    EXPECT_EQ(output.find("Price 100"), std::string::npos);
}

TEST(OrderBookTest, ZeroQuantityIsRejected) {
    OrderBook book;

    EXPECT_THROW(book.submit({1, Side::Buy, 100, 0}), std::invalid_argument);

    CoutCapture capture;
    book.print_book();
    const std::string output = capture.str();

    EXPECT_EQ(output.find("Price"), std::string::npos);
}

} // namespace
