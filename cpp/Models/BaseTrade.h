#ifndef BASETRADE_H
#define BASETRADE_H

#include "ITrade.h"
#include <chrono>
#include <string>

class BaseTrade : public ITrade {
public:
    BaseTrade() = default;
    virtual ~BaseTrade() = default;

    /**
     * After running command "head -n 1 cpp/Loaders/TradeData/BondTrades.dat | od -c | tail" 
     * This issue has been found across all trade data. 
     * The first line of each data file is 357 273 277 = EF BB BF UTF -8 BOM
     * causing an invisable failure in the test TestTradeLoadAccuracyOfFirstTrade. 
     * The fix is to remove the BOM from the data file.
     * Another fix which needs to happen is to remove \r which currently on every 
     * line in tradeId.
     */
    static void cleanField(std::string& s) {
        // Strip UTF-8 BOM (EF BB BF) if present at the start
        if (s.size() >= 3 &&
        static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB &&
        static_cast<unsigned char>(s[2]) == 0xBF) {
            s.erase(0, 3);
        }
        // remove \r carriage left by CRLF line earnings
        if (!s.empty() && s.back() == '\r') {
            s.pop_back();
        }
    }
    
    std::chrono::system_clock::time_point getTradeDate() const override { return tradeDate_; }
    void setTradeDate(const std::chrono::system_clock::time_point& date) override { tradeDate_ = date; }
    
    std::string getInstrument() const override { return instrument_; }
    void setInstrument(const std::string& instrument) override { instrument_ = instrument; }
    
    std::string getCounterparty() const override { return counterparty_; }
    void setCounterparty(const std::string& counterparty) override { counterparty_ = counterparty; }
    
    double getNotional() const override { return notional_; }
    void setNotional(double notional) override { notional_ = notional; }
    
    double getRate() const override { return rate_; }
    void setRate(double rate) override { rate_ = rate; }
    
    std::string getTradeType() const override = 0;
    std::string getTradeId() const override { return tradeId_; }
    
protected:
    std::string tradeId_;
    
private:
    std::chrono::system_clock::time_point tradeDate_;
    std::string instrument_;
    std::string counterparty_;
    double notional_ = 0.0;
    double rate_ = 0.0;
};

#endif // BASETRADE_H

