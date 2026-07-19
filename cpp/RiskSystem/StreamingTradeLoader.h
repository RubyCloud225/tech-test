#ifndef STREAMINGTRADELOADER_H
#define STREAMINGTRADELOADER_H

#include "../Loaders/IStreamingTradeLoader.h"
#include "../Models/ITrade.h"
#include "../Models/ITradeReceiver.h"
#include "../Models/IScalarResultReceiver.h"
#include "../Models/IPricingEngine.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

class StreamingTradeLoader : public ITradeReceiver {
private:
    std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders_;
    std::map<std::string, std::unique_ptr<IPricingEngine>> pricers_;
    IScalarResultReceiver* resultReceiver_ = nullptr; // Borrowed for the duration of loadAndPrice
    void loadPricers();

public:
    explicit StreamingTradeLoader(std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders);

    void loadAndPrice(IScalarResultReceiver* resultReceiver);
    void add(ITrade* trade) override;
};

#endif // STREAMINGTRADELOADER_H
