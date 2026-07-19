#include "ScreenResultPrinter.h"
#include <iostream>

void ScreenResultPrinter::printResults(const ScalarResults& results) {
    for (const auto& result : results) {
        // Write code here to print out the results such that we have:
        // TradeID : Result : Error
        // If there is no result then the output should be:
        // TradeID : Error
        // If there is no error the output should be:
        // TradeID : Result
        std::cout << result.getTradeId();
        if (result.getResult().has_value()) {
            std::cout << " : " << result.getResult().value();
        }
        if (result.getError().has_value()) {
            std::cout << " : " << result.getError().value();
        }
        std::cout << "\n";
    }
}
