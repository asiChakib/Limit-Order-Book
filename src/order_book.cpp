
#include "order_book.hpp"

#include <iostream>
#include <iterator>
#include <stdexcept>

void OrderBook::add_to_book(Order order) {
    if (order.side == Side::Buy) {
        auto& level = bids[order.price];
        level.orders.push_back(order);

        auto it = std::prev(level.orders.end());
        level.total_quantity += order.quantity;

        order_index.emplace(
            order.id, OrderLocation{order.side, order.price, it});
    } else {
        auto& level = asks[order.price];
        level.orders.push_back(order);

        auto it = std::prev(level.orders.end());
        level.total_quantity += order.quantity;

        order_index.emplace(
            order.id, OrderLocation{order.side, order.price, it});
    }
}

std::vector<Trade> OrderBook::submit(Order incoming) {
    std::vector<Trade> trades;

    if (incoming.quantity == 0) {
        throw std::invalid_argument("NO QUANTITY");
    }

    if (order_index.contains(incoming.id)) {
        throw std::invalid_argument("ORDER EXISTS");
    }

    if (incoming.side == Side::Buy) {
        while (incoming.quantity > 0 && !asks.empty()) {
            auto level_it = asks.begin();

            if (level_it->first > incoming.price) {
                break;
            }

            auto& level = level_it->second;
            auto resting_it = level.orders.begin();

            const auto traded_quantity =
                std::min(incoming.quantity, resting_it->quantity);

            trades.push_back(Trade{
                incoming.id,
                resting_it->id,
                resting_it->price,
                traded_quantity
            });

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
    } else {
        while (incoming.quantity > 0 && !bids.empty()) {
            auto level_it = bids.begin();

            if (level_it->first < incoming.price) {
                break;
            }

            auto& level = level_it->second;
            auto resting_it = level.orders.begin();

            const auto traded_quantity =
                std::min(incoming.quantity, resting_it->quantity);

            trades.push_back(Trade{
                resting_it->id,
                incoming.id,
                resting_it->price,
                traded_quantity
            });

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

    if (incoming.quantity > 0) {
        add_to_book(incoming);
    }

    return trades;
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
    } else {
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

std::optional<std::int64_t> OrderBook::best_bid() const {
    if (bids.empty()) {
        return std::nullopt;
    }

    return bids.begin()->first;
}

std::optional<std::int64_t> OrderBook::best_ask() const {
    if (asks.empty()) {
        return std::nullopt;
    }

    return asks.begin()->first;
}

std::optional<std::int64_t> OrderBook::spread() const {
    const auto bid = best_bid();
    const auto ask = best_ask();

    if (!bid || !ask) {
        return std::nullopt;
    }

    return *ask - *bid;
}

std::uint64_t OrderBook::quantity_at_price(
    Side side, std::int64_t price) const {
    if (side == Side::Buy) {
        const auto it = bids.find(price);

        if (it == bids.end()) {
            return 0;
        }

        return it->second.total_quantity;
    }

    const auto it = asks.find(price);

    if (it == asks.end()) {
        return 0;
    }

    return it->second.total_quantity;
}

void OrderBook::print_book() const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << "ASKS (lowest first)\n";

    for (const auto& [price, level] : asks) {
        std::cout << " Price " << price
                  << " | Total quantity " << level.total_quantity
                  << " | Orders: ";

        for (const auto& order : level.orders) {
            std::cout << '[' << order.id << ": "
                      << order.quantity << "] ";
        }

        std::cout << '\n';
    }

    std::cout << "BIDS (highest first)\n";

    for (const auto& [price, level] : bids) {
        std::cout << " Price " << price
                  << " | Total quantity " << level.total_quantity
                  << " | Orders: ";

        for (const auto& order : level.orders) {
            std::cout << '[' << order.id << ": "
                      << order.quantity << "] ";
        }

        std::cout << '\n';
    }
}
