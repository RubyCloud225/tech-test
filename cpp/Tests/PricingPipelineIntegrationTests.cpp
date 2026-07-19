// Integration tests comparing SerialPricer, StreamingTradeLoader, and
// ParallelPricer against each other, plus edge cases for each. These use the
// real pricing engines (real simulated delays, real config file), so unlike
// the fast unit tests in Tests/main.cpp this suite takes real time to run
// (order of a couple of minutes) - that's why it's built as its own separate
// IntegrationTests executable rather than folded into the fast Tests binary.
#include "TestFramework.h"
#include "../RiskSystem/SerialPricer.h"
#include "../RiskSystem/ParallelPricer.h"
#include "../RiskSystem/StreamingTradeLoader.h"
#include "../RiskSystem/ScreenResultPrinter.h"
#include "../Loaders/IStreamingTradeLoader.h"
#include "../Loaders/BondTradeLoader.h"
#include "../Loaders/FxTradeLoader.h"
#include "../Models/ScalarResults.h"
#include "../Models/BondTrade.h"
#include "../Models/FxTrade.h"
#include <iostream>
#include <memory>
#include <vector>

namespace {

void assertResultOnly(const ScalarResults& results, const std::string& tradeId) {
    ASSERT_TRUE(results.containsTrade(tradeId));
    auto result = results[tradeId];
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->getResult().has_value());
    ASSERT_FALSE(result->getError().has_value());
}

void assertErrorOnly(const ScalarResults& results, const std::string& tradeId, const std::string& expectedError) {
    ASSERT_TRUE(results.containsTrade(tradeId));
    auto result = results[tradeId];
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->getResult().has_value());
    ASSERT_TRUE(result->getError().has_value());
    ASSERT_EQ(result->getError().value(), expectedError);
}

void assertResultAndError(const ScalarResults& results, const std::string& tradeId) {
    ASSERT_TRUE(results.containsTrade(tradeId));
    auto result = results[tradeId];
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->getResult().has_value());
    ASSERT_TRUE(result->getError().has_value());
}

// One trade of each output shape the instructions describe: result+error,
// result only, and error only (from an unsupported trade type). GOV006/
// FWD001 are the engine's hardcoded forced error/warning trade IDs, reused
// here rather than adding new special-cased test data. Returns a fresh set
// of trade objects each call, since some pipelines (StreamingTradeLoader)
// take ownership and delete them.
std::vector<ITrade*> buildComparisonTrades() {
    return {
        new BondTrade("GOV006", BondTrade::GovBondTradeType),       // forced error, no result
        new BondTrade("PIPE-CORP1", BondTrade::CorpBondTradeType),  // result only
        new FxTrade("FWD001", FxTrade::FxForwardTradeType),         // result + warning error
        new BondTrade("PIPE-SUPRA1", BondTrade::SupraBondTradeType) // no pricing engine configured
    };
}

void assertComparisonTradeOutcomes(const ScalarResults& results) {
    assertErrorOnly(results, "GOV006", "Undefined error in pricing");
    assertResultOnly(results, "PIPE-CORP1");
    assertResultAndError(results, "FWD001");
    assertErrorOnly(results, "PIPE-SUPRA1", "No Pricing Engines available for this trade type");
}

// Minimal IStreamingTradeLoader backed by an in-memory list of trades, so
// tests can inject synthetic data into StreamingTradeLoader instead of it
// only ever being able to read the real data files.
class InMemoryTradeLoader : public IStreamingTradeLoader {
public:
    explicit InMemoryTradeLoader(std::vector<ITrade*> trades) : trades_(std::move(trades)) {}

    std::vector<ITrade*> loadTrades() override {
        return trades_;
    }

    void loadTrades(ITradeReceiver* receiver) override {
        for (ITrade* trade : trades_) {
            receiver->add(trade);
        }
    }

    std::string getDataFile() const override { return ""; }
    void setDataFile(const std::string&) override {}

private:
    std::vector<ITrade*> trades_;
};

} // namespace

// Runs the same four trades through both SerialPricer and ParallelPricer,
// and asserts they agree on which trades get which output shape.
TEST(TestSerialAndParallelAgreeOnTradeOutcomes) {
    std::vector<ITrade*> trades = buildComparisonTrades();
    std::vector<std::vector<ITrade*>> tradeContainers = { trades };

    ScalarResults serialResults;
    SerialPricer serialPricer;
    serialPricer.price(tradeContainers, &serialResults);

    ScalarResults parallelResults;
    ParallelPricer parallelPricer;
    parallelPricer.price(tradeContainers, &parallelResults);

    assertComparisonTradeOutcomes(serialResults);
    assertComparisonTradeOutcomes(parallelResults);

    std::cout << "\n--- SerialPricer output (TestSerialAndParallelAgreeOnTradeOutcomes) ---\n";
    ScreenResultPrinter().printResults(serialResults);
    std::cout << "--- ParallelPricer output (TestSerialAndParallelAgreeOnTradeOutcomes) ---\n";
    ScreenResultPrinter().printResults(parallelResults);

    for (ITrade* trade : trades) {
        delete trade;
    }
}

// Same comparison as above, but through StreamingTradeLoader via an injected
// in-memory loader - proving all three pipelines agree on the same trades,
// without StreamingTradeLoader needing to depend on the real data files.
// (StreamingTradeLoader deletes each trade once priced, so these trades are
// NOT deleted again here.)
TEST(TestStreamingAgreesWithSerialAndParallel) {
    std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders;
    loaders.push_back(std::make_unique<InMemoryTradeLoader>(buildComparisonTrades()));

    ScalarResults streamingResults;
    StreamingTradeLoader streamingLoader(std::move(loaders));
    streamingLoader.loadAndPrice(&streamingResults);

    assertComparisonTradeOutcomes(streamingResults);

    std::cout << "\n--- StreamingTradeLoader output (TestStreamingAgreesWithSerialAndParallel) ---\n";
    ScreenResultPrinter().printResults(streamingResults);
}

// A slower end-to-end smoke test wiring StreamingTradeLoader up to the real,
// configured trade data files (injected explicitly here, rather than
// hardcoded inside StreamingTradeLoader itself), confirming the real
// production wiring still works: GOV006 (forced error), FWD001 (result +
// warning), and GOV007 (a GovBond with no configured pricing engine).
TEST(TestStreamingHandlesRealConfiguredDataFiles) {
    std::vector<std::unique_ptr<IStreamingTradeLoader>> loaders;

    auto bondLoader = std::make_unique<BondTradeLoader>();
    bondLoader->setDataFile("TradeData/BondTrades.dat");
    loaders.push_back(std::move(bondLoader));

    auto fxLoader = std::make_unique<FxTradeLoader>();
    fxLoader->setDataFile("TradeData/FxTrades.dat");
    loaders.push_back(std::move(fxLoader));

    ScalarResults streamingResults;
    StreamingTradeLoader streamingLoader(std::move(loaders));
    streamingLoader.loadAndPrice(&streamingResults);

    assertErrorOnly(streamingResults, "GOV006", "Undefined error in pricing");
    assertResultAndError(streamingResults, "FWD001");
    assertErrorOnly(streamingResults, "GOV007", "No Pricing Engines available for this trade type");

    std::cout << "\n--- StreamingTradeLoader output (TestStreamingHandlesRealConfiguredDataFiles) ---\n";
    ScreenResultPrinter().printResults(streamingResults);
}

// price() with no trades at all should return cleanly with nothing recorded,
// for both pipelines.
TEST(TestEmptyTradePopulation) {
    std::vector<std::vector<ITrade*>> noTrades;

    ScalarResults serialResults;
    SerialPricer().price(noTrades, &serialResults);
    ASSERT_FALSE(serialResults.begin() != serialResults.end());

    ScalarResults parallelResults;
    ParallelPricer().price(noTrades, &parallelResults);
    ASSERT_FALSE(parallelResults.begin() != parallelResults.end());
}

// ParallelPricer holds one shared engine instance per trade type, so many
// GovBond trades priced at once all share the same BasePricingEngine::Random.
// This is a regression test for that generator being mutex-protected: without
// it, this reliably crashes or corrupts results under real thread contention.
TEST(TestParallelPricerHandlesSharedEngineUnderConcurrency) {
    constexpr int tradeCount = 20;
    std::vector<ITrade*> trades;
    for (int i = 0; i < tradeCount; ++i) {
        trades.push_back(new BondTrade("STRESS-GOV-" + std::to_string(i), BondTrade::GovBondTradeType));
    }
    std::vector<std::vector<ITrade*>> tradeContainers = { trades };

    ScalarResults results;
    ParallelPricer().price(tradeContainers, &results);

    for (int i = 0; i < tradeCount; ++i) {
        assertResultOnly(results, "STRESS-GOV-" + std::to_string(i));
    }

    for (ITrade* trade : trades) {
        delete trade;
    }
}

// Trade IDs are opaque std::string keys throughout (map lookups, printing),
// so multi-byte UTF-8 content should round-trip unchanged rather than being
// truncated or mis-split.
TEST(TestNonAsciiTradeIdRoundTrips) {
    const std::string tradeId = "G\xC3\x96V-\xE2\x98\x83-001"; // "GÖV-☃-001"
    std::vector<ITrade*> trades = { new BondTrade(tradeId, BondTrade::GovBondTradeType) };
    std::vector<std::vector<ITrade*>> tradeContainers = { trades };

    ScalarResults results;
    SerialPricer().price(tradeContainers, &results);

    assertResultOnly(results, tradeId);

    for (ITrade* trade : trades) {
        delete trade;
    }
}
