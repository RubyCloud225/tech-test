#ifndef SERIALTRADELOADER_H
#define SERIALTRADELOADER_H

#include "../Loaders/ITradeLoader.h"
#include "../Models/ITrade.h"
#include <vector>
#include <memory>

class SerialTradeLoader {
private:
    std::vector<std::unique_ptr<ITradeLoader>> loaders_;

public:
    explicit SerialTradeLoader(std::vector<std::unique_ptr<ITradeLoader>> loaders);

    std::vector<std::vector<ITrade*>> loadTrades();
};

#endif // SERIALTRADELOADER_H
