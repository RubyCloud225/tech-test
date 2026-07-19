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

