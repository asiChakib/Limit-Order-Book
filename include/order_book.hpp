
#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

enum class Side { Buy, Sell };

struct Order {
    std::uint64_t id;
    Side side;
    std::int64_t price;
    std::uint64_t quantity;
};

struct Trade {
    std::uint64_t buy_order_id;
    std::uint64_t sell_order_id;
    std::int64_t price;
    std::uint64_t quantity;
};

class OrderBook {
private:
    using OrderList = std::list<Order>;

    struct PriceLevel {
        OrderList orders;
        std::uint64_t total_quantity = 0;
    };

    using BidMap = std::map<std::int64_t, PriceLevel, std::greater<>>;
    using AskMap = std::map<std::int64_t, PriceLevel>;

    BidMap bids;
    AskMap asks;

    struct OrderLocation {
        Side side;
        std::int64_t price;
        OrderList::iterator order_it;
    };

    std::unordered_map<std::uint64_t, OrderLocation> order_index;

    void add_to_book(Order order);

public:
    std::vector<Trade> submit(Order order);

    [[nodiscard]] bool cancel(std::uint64_t order_id);

    [[nodiscard]] std::optional<std::int64_t> best_bid() const;
    [[nodiscard]] std::optional<std::int64_t> best_ask() const;
    [[nodiscard]] std::optional<std::int64_t> spread() const;

    [[nodiscard]] std::uint64_t quantity_at_price(
        Side side, std::int64_t price) const;

    void print_book() const;
};
