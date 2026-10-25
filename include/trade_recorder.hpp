
#pragma once

#include "order_book.hpp"

#include <vector>

class TradeRecorder {
public:
    void record(const std::vector<Trade>& trades);

    [[nodiscard]] const std::vector<Trade>& history() const;

private:
    std::vector<Trade> trades_;
};
