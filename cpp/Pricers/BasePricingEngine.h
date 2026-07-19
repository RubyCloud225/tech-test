#ifndef BASEPRICINGENGINE_H
#define BASEPRICINGENGINE_H

#include "../Models/IPricingEngine.h"
#include "../Models/ITrade.h"
#include "../Models/IScalarResultReceiver.h"
#include <map>
#include <string>
#include <random>
#include <mutex>

class BasePricingEngine : public IPricingEngine {
protected:
    BasePricingEngine();
    virtual ~BasePricingEngine() = default;
    
    void price(ITrade* trade, IScalarResultReceiver* resultReceiver) override;
    
public:
    bool isTradeTypeSupported(const std::string& tradeType) const;
    
protected:
    void addSupportedTradeType(const std::string& tradeType);
    int getDelay() const;
    void setDelay(int delay);
    virtual void priceTrade(ITrade* trade, IScalarResultReceiver* resultReceiver);
    virtual double calculateResult();
    
private:
    std::map<std::string, unsigned int> supportedTypes_;
    int delay_;
    
    // ParallelPricer may call nextDouble() for this same engine instance from
    // multiple threads at once (one shared engine per trade type), so access
    // to the mutable generator state is serialized with a mutex.
    class Random {
    public:
        Random();
        double nextDouble();
    private:
        std::random_device rd_;
        std::mt19937 gen_;
        std::uniform_int_distribution<unsigned int> dist_;
        std::mutex mutex_;
    };
    
    Random random_;
    
    static std::map<std::string, std::string>& getTradesToError();
    static std::map<std::string, std::string>& getTradesToWarn();
};

#endif // BASEPRICINGENGINE_H
