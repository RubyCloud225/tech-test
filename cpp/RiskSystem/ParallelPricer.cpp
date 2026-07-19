#include "ParallelPricer.h"
#include "PricingEngineFactory.h"
#include <chrono>
#include <thread>

namespace {

// AppleClang on macOS doesn't ship OpenMP support out of the box (it needs a
// separate `brew install libomp` plus extra compiler/linker flags), so this
// uses plain std::async/std::future instead - portable with no extra setup.
constexpr std::chrono::milliseconds kLaunchStagger{20};

// ScalarResults (the concrete IScalarResultReceiver used by ConsoleApp) isn't
// internally synchronized, since Serial/StreamingTradeLoader only ever write
// to it from one thread. This wrapper serializes access with resultMutex_ so
// pricing engines running on different threads can share a receiver safely,
// without requiring every IScalarResultReceiver implementation to lock itself.
class SynchronizedResultReceiver : public IScalarResultReceiver {
public:
    SynchronizedResultReceiver(IScalarResultReceiver& target, std::mutex& mutex)
        : target_(target), mutex_(mutex) {}

    void addResult(const std::string& tradeId, double result) override {
        std::lock_guard<std::mutex> lock(mutex_);
        target_.addResult(tradeId, result);
    }

    void addError(const std::string& tradeId, const std::string& error) override {
        std::lock_guard<std::mutex> lock(mutex_);
        target_.addError(tradeId, error);
    }

private:
    IScalarResultReceiver& target_;
    std::mutex& mutex_;
};

} // namespace

void ParallelPricer::loadPricers() {
    if (!pricers_.empty()) {
        return;
    }

    pricers_ = PricingEngineFactory::loadPricingEngines();
}

void ParallelPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers,
                           IScalarResultReceiver* resultReceiver) {
    loadPricers();

    SynchronizedResultReceiver synchronizedReceiver(*resultReceiver, resultMutex_);
    std::vector<std::future<void>> pending;

    for (const auto& tradeContainer : tradeContainers) {
        for (ITrade* trade : tradeContainer) {
            // Stagger launches slightly so a large trade population doesn't
            // spin up every thread in the same instant.
            std::this_thread::sleep_for(kLaunchStagger);

            pending.push_back(std::async(std::launch::async, [this, trade, &synchronizedReceiver]() {
                const std::string tradeType = trade->getTradeType();
                auto pricerIt = pricers_.find(tradeType);
                if (pricerIt == pricers_.end()) {
                    synchronizedReceiver.addError(trade->getTradeId(),
                                                  "No Pricing Engines available for this trade type");
                    return;
                }
                pricerIt->second->price(trade, &synchronizedReceiver);
            }));
        }
    }

    // Only returns once every task has completed, per the exercise's
    // requirement that price() blocks until all results are available.
    for (auto& future : pending) {
        future.get();
    }
}
