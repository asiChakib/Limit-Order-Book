
#include "order_book.hpp"
#include "trade_recorder.hpp"

#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <vector>

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book;
};

// 1. An empty book has no best prices or spread.
TEST_F(OrderBookTest, EmptyBookHasNoBestPrices) {
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.spread(), std::nullopt);

    EXPECT_EQ(book.quantity_at_price(Side::Buy, 100), 0);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 100), 0);
}

// 2. An unmatched order rests in the book.
TEST_F(OrderBookTest, UnmatchedOrderIsAddedToBook) {
    const auto trades = book.submit(Order{1, Side::Buy, 100, 10});

    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.quantity_at_price(Side::Buy, 100), 10);
}

// 3. An exact fill removes the resting order and records the trade.
TEST_F(OrderBookTest, ExactFillRemovesRestingOrder) {
    book.submit(Order{1, Side::Sell, 101, 5});

    const auto trades = book.submit(Order{2, Side::Buy, 101, 5});

    ASSERT_EQ(trades.size(), 1);

    EXPECT_EQ(trades[0].buy_order_id, 2);
    EXPECT_EQ(trades[0].sell_order_id, 1);
    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].quantity, 5);

    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 101), 0);
}

// 4. A partial fill leaves the unfilled quantity resting.
TEST_F(OrderBookTest, PartialFillLeavesRestingRemainder) {
    book.submit(Order{1, Side::Sell, 101, 10});

    const auto trades = book.submit(Order{2, Side::Buy, 101, 4});

    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 4);
    EXPECT_EQ(trades[0].price, 101);

    EXPECT_EQ(book.best_ask(), 101);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 101), 6);
}

// 5. Better-priced resting orders have priority.
TEST_F(OrderBookTest, BestPriceHasPriority) {
    book.submit(Order{1, Side::Sell, 103, 2});
    book.submit(Order{2, Side::Sell, 101, 2});
    book.submit(Order{3, Side::Sell, 102, 2});

    const auto trades = book.submit(Order{4, Side::Buy, 103, 4});

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].buy_order_id, 4);
    EXPECT_EQ(trades[0].sell_order_id, 2);
    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].quantity, 2);

    EXPECT_EQ(trades[1].buy_order_id, 4);
    EXPECT_EQ(trades[1].sell_order_id, 3);
    EXPECT_EQ(trades[1].price, 102);
    EXPECT_EQ(trades[1].quantity, 2);

    EXPECT_EQ(book.best_ask(), 103);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 103), 2);
}

// 6. Orders at the same price are matched in FIFO order.
TEST_F(OrderBookTest, FIFOIsPreservedAtSamePrice) {
    book.submit(Order{1, Side::Sell, 101, 2});
    book.submit(Order{2, Side::Sell, 101, 2});

    const auto trades = book.submit(Order{3, Side::Buy, 101, 3});

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].sell_order_id, 1);
    EXPECT_EQ(trades[0].quantity, 2);

    EXPECT_EQ(trades[1].sell_order_id, 2);
    EXPECT_EQ(trades[1].quantity, 1);

    EXPECT_EQ(book.quantity_at_price(Side::Sell, 101), 1);
}

// 7. One incoming order can match multiple price levels.
TEST_F(OrderBookTest, IncomingOrderCanMatchMultiplePriceLevels) {
    book.submit(Order{1, Side::Sell, 101, 2});
    book.submit(Order{2, Side::Sell, 102, 3});

    const auto trades = book.submit(Order{3, Side::Buy, 102, 4});

    ASSERT_EQ(trades.size(), 2);

    EXPECT_EQ(trades[0].sell_order_id, 1);
    EXPECT_EQ(trades[0].price, 101);
    EXPECT_EQ(trades[0].quantity, 2);

    EXPECT_EQ(trades[1].sell_order_id, 2);
    EXPECT_EQ(trades[1].price, 102);
    EXPECT_EQ(trades[1].quantity, 2);

    EXPECT_EQ(book.best_ask(), 102);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 102), 1);
}

// 8. Cancelling an existing order removes it from the book.
TEST_F(OrderBookTest, CancelExistingOrder) {
    book.submit(Order{1, Side::Buy, 100, 5});

    EXPECT_TRUE(book.cancel(1));

    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.quantity_at_price(Side::Buy, 100), 0);
}

// 9. Cancelling an unknown order fails without changing the book.
TEST_F(OrderBookTest, CancelUnknownOrder) {
    book.submit(Order{1, Side::Buy, 100, 5});

    EXPECT_FALSE(book.cancel(999));

    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.quantity_at_price(Side::Buy, 100), 5);
}

// 10. Queries return aggregate quantities and the correct spread.
TEST_F(OrderBookTest, BookQueriesReturnCorrectValues) {
    book.submit(Order{1, Side::Buy, 99, 5});
    book.submit(Order{2, Side::Buy, 100, 3});
    book.submit(Order{3, Side::Buy, 100, 7});
    book.submit(Order{4, Side::Sell, 105, 4});

    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_ask(), 105);
    EXPECT_EQ(book.spread(), 5);

    EXPECT_EQ(book.quantity_at_price(Side::Buy, 100), 10);
    EXPECT_EQ(book.quantity_at_price(Side::Buy, 99), 5);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 105), 4);

    EXPECT_EQ(book.quantity_at_price(Side::Buy, 101), 0);
    EXPECT_EQ(book.quantity_at_price(Side::Sell, 104), 0);
}

// 11. TradeRecorder accumulates executions across submissions.
TEST(TradeRecorderTest, AccumulatesTradesInOrder) {
    TradeRecorder recorder;

    const std::vector<Trade> first_batch{Trade{10, 1, 101, 5}, Trade{11, 2, 102, 3}};

    const std::vector<Trade> second_batch{Trade{12, 3, 103, 7}};

    recorder.record(first_batch);
    recorder.record(second_batch);

    const auto& history = recorder.history();

    ASSERT_EQ(history.size(), 3);

    EXPECT_EQ(history[0].buy_order_id, 10);
    EXPECT_EQ(history[0].sell_order_id, 1);
    EXPECT_EQ(history[0].price, 101);
    EXPECT_EQ(history[0].quantity, 5);

    EXPECT_EQ(history[1].buy_order_id, 11);
    EXPECT_EQ(history[1].sell_order_id, 2);

    EXPECT_EQ(history[2].buy_order_id, 12);
    EXPECT_EQ(history[2].sell_order_id, 3);
    EXPECT_EQ(history[2].price, 103);
    EXPECT_EQ(history[2].quantity, 7);
}

// Recording an empty batch must not change the history.
TEST(TradeRecorderTest, EmptyBatchLeavesHistoryUnchanged) {
    TradeRecorder recorder;

    const std::vector<Trade> trades{Trade{10, 1, 101, 5}};

    recorder.record(trades);

    const std::vector<Trade> empty_batch;
    recorder.record(empty_batch);

    ASSERT_EQ(recorder.history().size(), 1);
    EXPECT_EQ(recorder.history()[0].buy_order_id, 10);
    EXPECT_EQ(recorder.history()[0].quantity, 5);
}
