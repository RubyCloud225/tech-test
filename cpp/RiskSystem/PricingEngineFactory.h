#ifndef PRICINGENGINEFACTORY_H
#define PRICINGENGINEFACTORY_H

#include "../Models/IPricingEngine.h"
#include <map>
#include <memory>
#include <string>

// Shared by SerialPricer/StreamingTradeLoader/ParallelPricer to avoid each
// duplicating the trade-type-to-engine mapping.
class PricingEngineFactory {
public:
    static std::map<std::string, std::unique_ptr<IPricingEngine>> loadPricingEngines();
};

#endif // PRICINGENGINEFACTORY_H
