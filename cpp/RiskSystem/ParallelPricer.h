#ifndef PARALLELPRICER_H
#define PARALLELPRICER_H

#include "../Models/IPricingEngine.h"
#include "../Models/ITrade.h"
#include "../Models/IScalarResultReceiver.h"
#include "PricingConfigLoader.h"
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <future>

class ParallelPricer {
private:
    std::map<std::string, std::unique_ptr<IPricingEngine>> pricers_;
    std::mutex resultMutex_;

    void loadPricers();

public:
    void price(const std::vector<std::vector<ITrade*>>& tradeContainers,
               IScalarResultReceiver* resultReceiver);
};

#endif // PARALLELPRICER_H
