#include "order_book.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdexcept>

void OrderBook::add_to_book(Order order) {
    if (order.side == Side::Buy) {
        auto& level = bids[order.price];
        level.orders.push_back(order);

        auto it = std::prev(level.orders.end());
        level.total_quantity += order.quantity;

        order_index.emplace(order.id, OrderLocation{order.side, order.price, it});
    } else {
        auto& level = asks[order.price];
        level.orders.push_back(order);

        auto it = std::prev(level.orders.end());
        level.total_quantity += order.quantity;

        order_index.emplace(order.id, OrderLocation{order.side, order.price, it});
    }
}

void OrderBook::submit(Order incoming) {
    if (incoming.quantity == 0)
        throw std::invalid_argument("NO QUANTITY");
    if (order_index.contains(incoming.id))
        throw std::invalid_argument("ORDER EXISTS");
    if (incoming.side == Side::Buy) {
        while (incoming.quantity > 0 && !asks.empty()) {
            auto level_it = asks.begin();
            if (level_it->first > incoming.price)
                break; /*to be added to the book*/
            auto& level = level_it->second;
            auto resting_it = level.orders.begin();
            const auto traded_quantity =
                incoming.quantity < resting_it->quantity ? incoming.quantity : resting_it->quantity;
            std::cout << "Trade: buy order " << incoming.id << " vs sell order " << resting_it->id
                      << " at price " << resting_it->price << " for quantity of " << traded_quantity
                      << "\n";
            incoming.quantity -= traded_quantity;
            resting_it->quantity -= traded_quantity;
            level.total_quantity -= traded_quantity;
            if (resting_it->quantity == 0) {
                order_index.erase(resting_it->id);
                level.orders.erase(resting_it);
            }
            if (level.orders.empty()) {
                asks.erase(level_it);
            }
        }
    } else { /*incoming.side == Sell*/
        while (incoming.quantity > 0 && !bids.empty()) {
            auto level_it = bids.begin();

            if (level_it->first < incoming.price)
                break;
            auto& level = level_it->second;
            auto resting_it = level.orders.begin();
            const auto traded_quantity =
                incoming.quantity < resting_it->quantity ? incoming.quantity : resting_it->quantity;
            std::cout << "Trade: sell order " << incoming.id << " vs buy order " << resting_it->id
                      << " at price " << resting_it->price << " for quantity of " << traded_quantity
                      << "\n";

            incoming.quantity -= traded_quantity;
            resting_it->quantity -= traded_quantity;
            level.total_quantity -= traded_quantity;
            if (resting_it->quantity == 0) {
                order_index.erase(resting_it->id);
                level.orders.erase(resting_it);
            }
            if (level.orders.empty()) {
                bids.erase(level_it);
            }
        }
    }
    if (incoming.quantity > 0)
        add_to_book(incoming);
}

bool OrderBook::cancel(std::uint64_t order_id) {
    auto index_it = order_index.find(order_id);
    if (index_it == order_index.end()) {
        return false;
    }

    const auto location = index_it->second;
    if (location.side == Side::Buy) {
        auto level_it = bids.find(location.price);
        auto& level = level_it->second;
        level.total_quantity -= location.order_it->quantity;
        level.orders.erase(location.order_it);

        if (level.orders.empty()) {
            bids.erase(level_it);
        }
    } else { /*Cancel in Sell side*/
        auto level_it = asks.find(location.price);
        auto& level = level_it->second;
        level.total_quantity -= location.order_it->quantity;
        level.orders.erase(location.order_it);

        if (level.orders.empty()) {
            asks.erase(level_it);
        }
    }
    order_index.erase(index_it);
    return true;
}

void OrderBook::print_book() const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << "ASKS (lowest first)\n";
    for (const auto& [price, level] : asks) {
        std::cout << " Price " << price << " | Total quantity " << level.total_quantity
                  << " | Orders: ";
        for (const auto& order : level.orders) {
            std::cout << '[' << order.id << ": " << order.quantity << "]";
        }
        std::cout << '\n';
    }
    std::cout << "BIDS (highest first)\n";
    for (const auto& [price, level] : bids) {
        std::cout << " Price " << price << " | Total quantity " << level.total_quantity
                  << " | Orders: ";

        for (const auto& order : level.orders) {
            std::cout << '[' << order.id << ": " << order.quantity << "] ";
        }
        std::cout << '\n';
    }
}
