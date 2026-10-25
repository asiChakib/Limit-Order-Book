#include "order_book.hpp"

#include <iostream>

int main() {
    OrderBook book;

    book.submit({1, Side::Sell, 101, 5});
    book.submit({2, Side::Sell, 102, 10});
    book.submit({3, Side::Buy, 100, 8});

    book.print_book();

    std::cout << "\nSubmitting buy order at 102...\n";
    book.submit({4, Side::Buy, 102, 8});

    book.print_book();

    std::cout << "\nCancelling order 2...\n";
    if (book.cancel(2)) {
        std::cout << "Order cancelled\n";
    } else {
        std::cout << "Order not found\n";
    }

    book.print_book();
}
