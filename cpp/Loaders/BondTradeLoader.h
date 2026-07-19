#ifndef BONDTRADELOADER_H
#define BONDTRADELOADER_H

#include "IStreamingTradeLoader.h"
#include "../Models/BondTrade.h"
#include "../Models/BondTradeList.h"
#include <string>
#include <vector>
#include <memory>

class BondTradeLoader : public IStreamingTradeLoader {
private:
    static constexpr char separator = ',';
    std::string dataFile_;

    BondTrade* createTradeFromLine(const std::string line);
    void loadTradesFromFile(const std::string filename, ITradeReceiver& receiver);

public:
    std::vector<ITrade*> loadTrades() override;
    void loadTrades(ITradeReceiver* receiver) override;
    std::string getDataFile() const override;
    void setDataFile(const std::string& file) override;
};

#endif // BONDTRADELOADER_H
