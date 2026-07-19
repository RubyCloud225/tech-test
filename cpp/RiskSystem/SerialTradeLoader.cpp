#include "SerialTradeLoader.h"

SerialTradeLoader::SerialTradeLoader(std::vector<std::unique_ptr<ITradeLoader>> loaders)
    : loaders_(std::move(loaders)) {
}

std::vector<std::vector<ITrade*>> SerialTradeLoader::loadTrades() {
    std::vector<std::vector<ITrade*>> result;

    for (const auto& loader : loaders_) {
        result.push_back(loader->loadTrades());
    }

    return result;
}
