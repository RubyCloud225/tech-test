#include "StreamingTradeLoader.h"
#include "PricingEngineFactory.h"
#include <stdexcept>

StreamingTradeLoader::StreamingTradeLoader(std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders)
    : loaders_(std::move(loaders)) {
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

    for (const auto& loader : loaders_) {
        loader->loadTrades(this);
    }

    resultReceiver_ = nullptr;
}
