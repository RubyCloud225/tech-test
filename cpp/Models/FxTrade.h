#ifndef FXTRADE_H
#define FXTRADE_H

#include "BaseTrade.h"
#include <chrono>
#include <stdexcept>

class FxTrade : public BaseTrade {
public:
    static constexpr const char* FxSpotTradeType = "FxSpot";
    static constexpr const char* FxForwardTradeType = "FxFwd";

    FxTrade(const std::string& tradeId, const std::string& tradeType)
        : tradeType_(tradeType) {
        if (!tradeId.empty()) {
            tradeId_ = tradeId;
            cleanField(tradeId_); cleanField(tradeType_);
            if (tradeId.empty()) {
                throw std::invalid_argument("A valid non null, non empty tradeId must be provided");
            }
        }
    }
    
    std::string getTradeType() const override {
        return tradeType_;
    }
    
    std::chrono::system_clock::time_point getValueDate() const { return valueDate_; }
    void setValueDate(const std::chrono::system_clock::time_point& date) { valueDate_ = date; }
    
private:
    std::string tradeType_;
    std::chrono::system_clock::time_point valueDate_;
};

#endif // FXTRADE_H

