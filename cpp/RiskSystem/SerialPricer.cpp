#include "SerialPricer.h"
#include "PricingEngineFactory.h"

void SerialPricer::loadPricers() {
    // Adding in the ability to just load and replace over and above
    // the offloading of memory
    if (!pricers_.empty()) {
        return;
    }

    pricers_ = PricingEngineFactory::loadPricingEngines();
}

void SerialPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers,
                         IScalarResultReceiver* resultReceiver) {
    loadPricers();

    for (const auto& tradeContainer : tradeContainers) {
        for (ITrade* trade : tradeContainer) {
            const std::string tradeType = trade->getTradeType();
            auto pricer = pricers_.find(tradeType);
            if (pricer == pricers_.end()) {
                resultReceiver->addError(trade->getTradeId(),
                                         "No Pricing Engines available for this trade type");
                continue;
            }
            pricer->second->price(trade, resultReceiver);
        }
    }
}
