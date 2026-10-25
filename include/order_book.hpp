
#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <unordered_map>

enum class Side { Buy, Sell };

struct Order {
    std::uint64_t id;
    Side side;
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
    void submit(Order incoming);

    [[nodiscard]] bool cancel(std::uint64_t order_id);

    void print_book() const;
};
