#ifndef ISTREAMINGTRADELOADER_H
#define ISTREAMINGTRADELOADER_H

#include "ITradeLoader.h"
#include "../Models/ITradeReceiver.h"

// Streaming variant of ITradeLoader: pushes each parsed trade to a receiver
// immediately, so the full trade population never has to be held in memory.
class IStreamingTradeLoader : public ITradeLoader {
public:
    virtual void loadTrades(ITradeReceiver* receiver) = 0;
};

#endif // ISTREAMINGTRADELOADER_H
