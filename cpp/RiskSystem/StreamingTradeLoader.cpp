#include "StreamingTradeLoader.h"
#include "../Loaders/BondTradeLoader.h"
#include "../Loaders/FxTradeLoader.h"
#include "PricingEngineFactory.h"
#include <stdexcept>

std::vector<std::unique_ptr<IStreamingTradeLoader>> StreamingTradeLoader::getTradeLoaders() {
    std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders;

    auto bondLoader = std::make_unique<BondTradeLoader>();
    bondLoader->setDataFile("TradeData/BondTrades.dat");
    loaders.push_back(std::move(bondLoader));

    auto fxLoader = std::make_unique<FxTradeLoader>();
    fxLoader->setDataFile("TradeData/FxTrades.dat");
    loaders.push_back(std::move(fxLoader));

    return loaders;
}

void StreamingTradeLoader::loadPricers() {
    if (!pricers_.empty()) {
        return;
    }

    pricers_ = PricingEngineFactory::loadPricingEngines();
}

void StreamingTradeLoader::add(ITrade* trade) {
    const std::string tradeType = trade->getTradeType();
    auto pricer = pricers_.find(tradeType);
    if (pricer == pricers_.end()) {
        resultReceiver_->addError(trade->getTradeId(),
                                  "No Pricing Engines available for this trade type");
    } else {
        pricer->second->price(trade, resultReceiver_);
    }
    // The trade is finished with once priced. Destroying it here is what keeps
    // memory bounded to a single trade regardless of the size of the population.
    delete trade;
}

void StreamingTradeLoader::loadAndPrice(IScalarResultReceiver* resultReceiver) {
    resultReceiver_ = resultReceiver;
    loadPricers();

    for (const auto& loader : getTradeLoaders()) {
        loader->loadTrades(this);
    }

    resultReceiver_ = nullptr;
}
