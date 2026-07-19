#include "SerialPricer.h"
#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"
#include "../Models/BondTrade.h"
#include "../Models/FxTrade.h"
#include <stdexcept>


/**
 * Unneeded due to loading the engine once and reused.
 * price() will reload configuration and replace the engine instead
 */
// SerialPricer::~SerialPricer() {
//}

void SerialPricer::loadPricers() {
    // Adding in the ability to just load and replace over and above
    // the offloading of memory
    if (!pricers_.empty()) {
        return;
    }

    PricingConfigLoader pricingConfigLoader;
    pricingConfigLoader.setConfigFile("./PricingConfig/PricingEngines.xml");
    PricingEngineConfig pricerConfig = pricingConfigLoader.loadConfig();
    
    for (const auto& configItem : pricerConfig) {
        const std::string& tradeType = configItem.getTradeType();
        // configure the names as cannot instantiate a class so it needs
        // to map directly to the corresponding engine
        std::unique_ptr<IPricingEngine> engine;
        if (tradeType == BondTrade::GovBondTradeType) {
            engine = std::make_unique<GovBondPricingEngine>();
        } else if (tradeType == BondTrade::CorpBondTradeType) {
            engine = std::make_unique<CorpBondPricingEngine>();
        } else if (tradeType == FxTrade::FxSpotTradeType ||
                   tradeType == FxTrade::FxForwardTradeType) {
            engine = std::make_unique<FxPricingEngine>();
        } else {
            throw std::runtime_error("No pricing engine available for trade type: " + tradeType);
        }
        pricers_[tradeType] = std::move(engine);
    }
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
