#include "../RiskSystem/SerialTradeLoader.h"
#include "../Loaders/BondTradeLoader.h"
#include "../Loaders/FxTradeLoader.h"
#include "../Models/ScalarResults.h"
#include "../RiskSystem/SerialPricer.h"
#include "../RiskSystem/ParallelPricer.h"
#include "../RiskSystem/ScreenResultPrinter.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

int main(int argc, char* argv[]) {
    std::vector<std::unique_ptr<ITradeLoader>> loaders;

    auto bondLoader = std::make_unique<BondTradeLoader>();
    bondLoader->setDataFile("TradeData/BondTrades.dat");
    loaders.push_back(std::move(bondLoader));

    auto fxLoader = std::make_unique<FxTradeLoader>();
    fxLoader->setDataFile("TradeData/FxTrades.dat");
    loaders.push_back(std::move(fxLoader));

    SerialTradeLoader tradeLoader(std::move(loaders));
    auto allTrades = tradeLoader.loadTrades();
    
    ScalarResults results;
    SerialPricer pricer;
    pricer.price(allTrades, &results);
    
    ScreenResultPrinter screenPrinter;
    screenPrinter.printResults(results);
    
    std::cout << "Press any key to exit.." << std::endl;
    _getch();
    
    return 0;
}
