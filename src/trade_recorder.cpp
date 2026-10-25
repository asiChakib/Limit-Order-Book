
#include "trade_recorder.hpp"

void TradeRecorder::record(const std::vector<Trade>& trades) {
    trades_.insert(trades_.end(), trades.begin(), trades.end());
}

const std::vector<Trade>& TradeRecorder::history() const {
    return trades_;
}
