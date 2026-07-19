#ifndef FXTRADELOADER_H
#define FXTRADELOADER_H

#include "IStreamingTradeLoader.h"
#include "../Models/FxTrade.h"
#include <string>
#include <vector>

class FxTradeLoader : public IStreamingTradeLoader {
private:
    std::string dataFile_;

    void loadTradesFromFile(const std::string filename, ITradeReceiver& receiver);

public:
    FxTrade* createTradeFromLine(const std::string line);
    // NOTE: These methods are only here to allow the solution to compile prior to the test being completed.
    std::vector<ITrade*> loadTrades() override;
    void loadTrades(ITradeReceiver* receiver) override;
    std::string getDataFile() const override;
    void setDataFile(const std::string& file) override;
};

#endif // FXTRADELOADER_H
