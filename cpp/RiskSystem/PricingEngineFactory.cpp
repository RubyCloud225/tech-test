#include "PricingEngineFactory.h"
#include "PricingConfigLoader.h"
#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"
#include "../Models/BondTrade.h"
#include "../Models/FxTrade.h"
#include <stdexcept>

std::map<std::string, std::unique_ptr<IPricingEngine>> PricingEngineFactory::loadPricingEngines() {
    std::map<std::string, std::unique_ptr<IPricingEngine>> pricers;

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
        pricers[tradeType] = std::move(engine);
    }

    return pricers;
}
