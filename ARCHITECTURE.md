# Architecture

**Author:** Catherine Earl
**Date:** 2026-07-20

## Summary

This is a simplified risk system: it loads trades from one or more feed files (government/corporate bonds, FX spot/forward), prices each one using a pricing engine selected by trade type, and reports a result or error per trade. Which C++ class prices which trade type is not hardcoded — it's read at runtime from `PricingEngines.xml` via `PricingConfigLoader`, and turned into engine instances by `PricingEngineFactory`.

The codebase is organized in four layers:
- **Models** — trade domain types and the receiver/result interfaces, with no I/O.
- **Loaders** — reads raw feed files into `ITrade` objects, either as a batch (`ITradeLoader::loadTrades()`) or streamed one at a time (`IStreamingTradeLoader::loadTrades(ITradeReceiver*)`).
- **Pricers** — concrete pricing engines, all sharing simulated delay/RNG/forced-error behavior via `BasePricingEngine`.
- **RiskSystem** — orchestration: three interchangeable pricing strategies (serial, streaming, parallel) that only differ in *how* trades are loaded and priced, not in the pricing logic itself.

Trade data sources are injected via constructor (not hardcoded inside the loader classes), so `ConsoleApp/main.cpp` is the only place that knows about real file paths — everything else can be pointed at synthetic data for testing.

## Directory tree

```
cpp/
├── CMakeLists.txt                  Build config: Models/Loaders/Pricers/RiskSystem libs,
│                                    ConsoleApp + Tests + IntegrationTests executables
│
├── ConsoleApp/
│   └── main.cpp                    Composition root - builds BondTradeLoader/FxTradeLoader
│                                    pointed at real data files, injects them into
│                                    SerialTradeLoader, prices via SerialPricer, prints results
│
├── Models/                         Trade domain types + result/receiver interfaces (no I/O)
│   ├── ITrade.h                    Trade interface: getTradeId/getTradeType/getInstrument/...
│   ├── BaseTrade.h                 Shared trade fields + BOM/CR field cleaning (cleanField)
│   ├── BondTrade.h                 GovBond / CorpBond / Supra trade types
│   ├── FxTrade.h                   FxSpot / FxFwd trade type + value date
│   ├── ITradeReceiver.h            Receives trades one at a time: add(ITrade*)
│   ├── TradeList.h                 ITradeReceiver that owns trades; release() transfers
│   │                                ownership out before the list's destructor would delete them
│   ├── BondTradeList.h             TradeList specialization used by BondTradeLoader
│   ├── IPricingEngine.h            Pricing engine interface: price(trade, resultReceiver)
│   ├── IScalarResultReceiver.h     Receives priced results/errors per trade
│   ├── ScalarResult.h              One trade's (optional result, optional error)
│   └── ScalarResults.{h,cpp}       IScalarResultReceiver impl; iterable (range-for) collection
│                                    of ScalarResult, merging the results_/errors_ maps
│
├── Loaders/                        Reads raw trade data files into ITrade objects
│   ├── ITradeLoader.h              loadTrades() -> vector<ITrade*>              (batch)
│   ├── IStreamingTradeLoader.h     + loadTrades(ITradeReceiver*)                (streaming)
│   ├── BondTradeLoader.{h,cpp}     Parses BondTrades.dat (comma-separated)
│   ├── FxTradeLoader.{h,cpp}       Parses FxTrades.dat (¬-separated)
│   └── TradeData/                  Raw trade feed files
│       ├── BondTrades.dat
│       └── FxTrades.dat
│
├── Pricers/                         Concrete pricing engines
│   ├── BasePricingEngine.{h,cpp}   Shared simulated pricing: delay, mutex-protected RNG,
│   │                                hardcoded forced error/warning trade IDs (GOV006, FWD001)
│   ├── GovBondPricingEngine.h      5s delay, supports GovBond
│   ├── CorpBondPricingEngine.h     8s delay, supports CorpBond
│   └── FxPricingEngine.h           2s delay, supports FxSpot + FxFwd
│
├── RiskSystem/                     Orchestration: config, trade sourcing, pricing, output
│   ├── PricingEngineConfigItem.h   One <Engine> row (tradeType / assembly / typeName)
│   ├── PricingEngineConfig.h       vector<PricingEngineConfigItem>
│   ├── PricingConfigLoader.{h,cpp} Parses PricingEngines.xml
│   ├── PricingConfig/
│   │   └── PricingEngines.xml      tradeType -> pricing engine class name mapping
│   ├── PricingEngineFactory.{h,cpp}  Builds {tradeType: unique_ptr<IPricingEngine>} from
│   │                                  config; shared by all three pricers below
│   │
│   ├── SerialTradeLoader.{h,cpp}  Loads all trades from injected ITradeLoaders upfront
│   ├── SerialPricer.{h,cpp}       Prices an already-loaded trade population sequentially
│   │
│   ├── StreamingTradeLoader.{h,cpp}  Loads + prices one trade at a time via injected
│   │                                  IStreamingTradeLoaders; deletes each trade right after
│   │                                  pricing it, so memory is bounded to a single trade
│   │
│   ├── ParallelPricer.{h,cpp}     Prices an already-loaded trade population concurrently
│   │                               (std::async/std::future), synchronizing writes into the
│   │                               shared result receiver
│   │
│   └── ScreenResultPrinter.{h,cpp}  Prints "TradeID : Result : Error" lines
│
└── Tests/
    ├── TestFramework.h             Minimal TEST()/ASSERT_* macros + test runner
    ├── main.cpp                    Fast unit test binary (`Tests`)
    ├── BondTradeLoaderTests.cpp
    ├── FxTradeLoaderTests.cpp
    ├── PricingConfigLoaderTests.cpp
    ├── PricingEngineTests.cpp
    ├── ScalarResultsTests.cpp
    ├── IntegrationTestsMain.cpp    Slower integration test binary (`IntegrationTests`)
    └── PricingPipelineIntegrationTests.cpp  Cross-pipeline comparison (Serial/Streaming/
                                              Parallel agree on the same trades) + edge cases
```

## Key relationships

- **Trades**: `ITrade` ← `BaseTrade` ← `BondTrade` / `FxTrade`.
- **Loading**: `ITradeLoader` ← `IStreamingTradeLoader` ← `BondTradeLoader` / `FxTradeLoader` (both implement the streaming interface, so the same parsing code serves batch and streaming loads via a generic `ITradeReceiver&` parameter).
- **Pricing engines**: `IPricingEngine` ← `BasePricingEngine` ← `GovBondPricingEngine` / `CorpBondPricingEngine` / `FxPricingEngine`.
- **Three pricing strategies, one engine set**: `SerialPricer`, `StreamingTradeLoader`, and `ParallelPricer` each build their `{tradeType: engine}` map via the same `PricingEngineFactory`, and differ only in *when* trades are loaded relative to pricing, and whether pricing happens sequentially or concurrently.
- **Dependency injection**: `SerialTradeLoader` and `StreamingTradeLoader` receive their loaders (already pointed at a data source) via constructor, rather than constructing `BondTradeLoader`/`FxTradeLoader` internally with hardcoded paths — real paths are only known in `ConsoleApp/main.cpp` and in test fixtures.
