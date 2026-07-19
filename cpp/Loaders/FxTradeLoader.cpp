#include "FxTradeLoader.h"
#include "../Models/TradeList.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

// NOTE: These methods are only here to allow the solution to compile prior to the test being completed.

FxTrade* FxTradeLoader::createTradeFromLine(const std::string line) {
    std::vector<std::string> items;
    std::size_t start = 0, pos;
    static const std::string separator = "¬"; // 2 bytes (0xC2 0xAC), removal of a non char

    while ((pos = line.find(separator, start)) != std::string::npos) {
        items.push_back(line.substr(start, pos - start));
        start = pos + separator.size(); // advance by 2.
    }
    items.push_back(line.substr(start)); // last tradeId
    if (items.size() < 9) {
        throw std::runtime_error("Invalid line format");
    }

    FxTrade* trade = new FxTrade(items[8], items[0]);

    // Trade date (field 1)
    std::tm tm = {};
    std::istringstream dateStream(items[1]);
    dateStream >> std::get_time(&tm, "%Y-%m-%d");
    trade->setTradeDate(std::chrono::system_clock::from_time_t(std::mktime(&tm)));
    trade->setInstrument(items[2] + items[3]); // concat ccy1 and ccy2
    trade->setCounterparty(items[7]);
    trade->setNotional(std::stod(items[4]));
    trade->setRate(std::stod(items[5]));

    // value date - FX specific second date
    std::tm vtm = {};
    std::istringstream valueStream(items[6]);
    valueStream >> std::get_time(&vtm, "%Y-%m-%d");
    trade->setValueDate(std::chrono::system_clock::from_time_t(std::mktime(&vtm)));

    return trade;
}

void FxTradeLoader::loadTradesFromFile(const std::string filename, ITradeReceiver& receiver) {
    if (filename.empty()) {
        throw std::invalid_argument("Filename cannot be null");
    }

    std::ifstream stream(filename);
    if(!stream.is_open()) {
        throw std::runtime_error("Cannot open file : " + filename);
    }

    int lineCount = 0;
    std::string line;

    while (std::getline(stream, line)) {
        if (lineCount < 2) {
            // Skip the two header lines: title and column names
            lineCount++;
            continue;
        }

        // Footer at END¬5 terminates
        if (line.rfind("END", 0) == 0) {
            break;
        }

        receiver.add(createTradeFromLine(line));
        lineCount++;
    }
}

std::vector<ITrade*> FxTradeLoader::loadTrades() {
    TradeList tradeList;
    loadTradesFromFile(dataFile_, tradeList);
    return tradeList.release();
}

void FxTradeLoader::loadTrades(ITradeReceiver* receiver) {
    loadTradesFromFile(dataFile_, *receiver);
}

std::string FxTradeLoader::getDataFile() const {
    return dataFile_;
}

void FxTradeLoader::setDataFile(const std::string& file) {
    dataFile_ = file;
}
