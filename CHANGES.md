# Changes since forking

**Author:** Catherine Earl
**Date:** 2026-07-20

Base fork point: `676b4df` (tip of `upstream/main` at fork time — "Update candidate instructions for test time expectations").

Current status: **all 8 exercises implemented.** Full build is clean; the fast unit test suite (`./Tests`) passes 16/16; a separate, slower integration suite (`./IntegrationTests`, added as part of this work) passes 6/6.

## Exercise 1 — BondTradeLoader
- Fixed a missing `lineCount++` in `loadTradesFromFile` that left the header-skip counter stuck at 0.
- Fixed a double-free/use-after-free: `loadTrades()` used to copy raw `ITrade*` pointers out of a local `BondTradeList` into a new vector, leaving both the local list and the returned vector owning the same pointers — the local list's destructor then deleted trades the caller still held. Added `TradeList::release()` (ownership-transfer via `swap`) and switched `loadTrades()` to use it.

## Exercise 2 — FxTradeLoader
- Implemented parsing of `FxTrades.dat` (`¬`-separated fields), with `Instrument` built as `Ccy1 + Ccy2` per the spec.
- Shared the BOM/CR field-cleaning logic (`BaseTrade::cleanField`) between Bond and FX trades.

## Exercise 3 — PricingConfigLoader
- Implemented hand-rolled XML attribute parsing of `PricingEngines.xml` into `PricingEngineConfig` (a `vector<PricingEngineConfigItem>`), with validation for missing/unreadable config files.

## Exercise 4 — SerialPricer
- Implemented `loadPricers()` to map each configured trade type to a concrete pricing engine (`GovBondPricingEngine`, `CorpBondPricingEngine`, `FxPricingEngine`), owned via `unique_ptr` (previously a raw-pointer map with an empty destructor — a guaranteed leak once populated).
- This trade-type-to-engine mapping was later extracted into a shared `PricingEngineFactory` (see Exercise 8) so it isn't duplicated across pricers.

## Exercise 5 — ScalarResults iterator
- The header was missing the `Iterator`'s member variables (`parent_`, `resultIt_`, `errorIt_`) entirely, and its private constructor's first parameter was typed `const ScalarResult*` (a single result) instead of `const ScalarResults*` (the container) — the out-of-line `.cpp` definitions didn't match any declaration, so it didn't compile.
- `skipDuplicates()` also compared an iterator into `errors_` against `results_.end()` (a different map's iterator type/values) instead of `errors_.end()`.
- Fixed both; iterator now correctly walks `results_` to exhaustion then `errors_`, skipping trades already yielded.

## Exercise 6 — ScreenResultPrinter
- Implemented `printResults()` to print `TradeID : Result : Error`, `TradeID : Result`, or `TradeID : Error` depending on which of the optional fields are present.

## Exercise 7 — StreamingTradeLoader
- Added a new `IStreamingTradeLoader` interface (extends `ITradeLoader` with `loadTrades(ITradeReceiver*)`), rather than overloading the receiver-facing `ITradeReceiver` interface with loading concerns (an earlier in-progress attempt had put `loadTrades()` directly on `ITradeReceiver`, which made every receiver, e.g. `TradeList`, abstract and broke the build).
- `BondTradeLoader`/`FxTradeLoader` now implement `IStreamingTradeLoader`: their file-parsing helper was generalized from a concrete list type to a generic `ITradeReceiver&`, so the same parsing code serves both the batch `loadTrades()` and the new streaming `loadTrades(ITradeReceiver*)` (which pushes each trade to the receiver as it's parsed, one at a time).
- `StreamingTradeLoader::add()` prices each trade immediately and `delete`s it right after, so memory is bounded to one trade at a time regardless of population size.

## Exercise 8 — ParallelPricer
- Implemented `price()` using `std::async`/`std::future` (one task per trade, staggered by 20ms so a large population doesn't spin up every thread at once), blocking on every future before returning. A comment notes why `std::async` was chosen over OpenMP: AppleClang on macOS doesn't ship OpenMP support without `brew install libomp` plus extra compiler/linker flags.
- Added a `SynchronizedResultReceiver` wrapper (mutex-protected) since the shared `IScalarResultReceiver` isn't internally synchronized and multiple pricing threads write results/errors into it concurrently.
- Fixed two real concurrency bugs surfaced by running pricing engines on multiple threads for the first time:
  - `BasePricingEngine::Random` held one `mt19937` per engine instance, and `ParallelPricer` shares one engine instance per trade type across all trades of that type — concurrent calls were racing on the same generator state. Now mutex-protected.
  - `getTradesToError()`/`getTradesToWarn()` lazily populated `static std::map`s guarded only by a plain `bool`, not synchronized — two threads could race on first call. Switched to a static initializer (thread-safe via magic statics).
- Verified: the four trade groups price concurrently in ~8.5s (bounded by the single slowest trade), vs. 50s+ serially, with identical correct results.

## Cross-cutting changes
- **Removed hardcoded trade data paths.** `SerialTradeLoader` and `StreamingTradeLoader` used to construct their own `BondTradeLoader`/`FxTradeLoader` internally with `"TradeData/BondTrades.dat"`/`"TradeData/FxTrades.dat"` baked in. Both now take their loaders via constructor injection; `ConsoleApp/main.cpp` builds and injects them explicitly. This also fixed a leak — `SerialTradeLoader::getTradeLoaders()` `new`'d loaders that were never deleted; they're now `unique_ptr`-owned.
- **New `IntegrationTests` executable** (`cpp/Tests/PricingPipelineIntegrationTests.cpp` + `IntegrationTestsMain.cpp`), separate from the fast `Tests` binary since it exercises real pricing delays (~2 minutes total):
  - Confirms `SerialPricer`, `ParallelPricer`, and `StreamingTradeLoader` (via an injected in-memory test double) all agree on the same trades across all three result-shape cases (result+error, result-only, error-only from an unsupported trade type).
  - A slower end-to-end smoke test wiring `StreamingTradeLoader` to the real data files.
  - Edge cases: empty trade population, 20 same-type trades priced concurrently through one shared engine instance (regression test for the `Random` mutex fix), and a non-ASCII trade ID round-trip.
