#ifndef BONDTRADE_H
#define BONDTRADE_H

#include "BaseTrade.h"
#include <stdexcept>

class BondTrade : public BaseTrade {
public:
    static constexpr const char* GovBondTradeType = "GovBond";
    static constexpr const char* CorpBondTradeType = "CorpBond";
    // Supra bond type is not used in the data file but is included for completeness
    static constexpr const char* SupraBondTradeType = "Supra";

    /**
     * After running command "head -n 1 cpp/Loaders/TradeData/BondTrades.dat | od -c | tail" 
     * the first line of the data file is 357 273 277 = EF BB BF UTF -8 BOM
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

    /** 
     * The constructor below takes a trade ID and defaults it to GovBondTradeType
     * Data shows three types of data GovBond, CorpBond and Supra
     * The fix is to remove the GoveBondTradeType default and require the trade type to be passed in as a parameter
     */
    BondTrade(const std::string& tradeId, const std::string& tradeType)
        
        : tradeType_(tradeType){
        // if tradeId_ is not initialised at the beginning so cleaning
        // the data does not equal cleaning an empty field
        tradeId_ = tradeId;
        cleanField(tradeId_); cleanField(tradeType_);
        // Validate trade ID if null return an invalid argument exception
        if (tradeId.empty()) {
            throw std::invalid_argument("A valid non null, non empty trade ID must be provided");
        }
    }
    std::string getTradeType() const override { return tradeType_; }
    
private:
    std::string tradeType_;
};

#endif // BONDTRADE_H

