#include "ScalarResults.h"
#include <stdexcept>

ScalarResults::~ScalarResults() = default;

std::optional<ScalarResult> ScalarResults::operator[](const std::string& tradeId) const {
    if (!containsTrade(tradeId)) {
        return std::nullopt;
    }

    std::optional<double> priceResult = std::nullopt;
    std::optional<std::string> error = std::nullopt;

    auto resultIt = results_.find(tradeId);
    if (resultIt != results_.end()) {
        priceResult = resultIt->second;
    }

    auto errorIt = errors_.find(tradeId);
    if (errorIt != errors_.end()) {
        error = errorIt->second;
    }

    return ScalarResult(tradeId, priceResult, error);
}

bool ScalarResults::containsTrade(const std::string& tradeId) const {
    return results_.find(tradeId) != results_.end() || errors_.find(tradeId) != errors_.end();
}

void ScalarResults::addResult(const std::string& tradeId, double result) {
    results_[tradeId] = result;
}

void ScalarResults::addError(const std::string& tradeId, const std::string& error) {
    errors_[tradeId] = error;
}

/**
 * Results and errors for a trade are held in two separate maps, a trade must be 
 * enumerated only once, carrying whichever of the two it has. The iterator therefor
 * carries results_ to exhaustion and then continues through errors_, skipping any trade
 * already seen in results_.
 */
ScalarResults::Iterator::Iterator(const ScalarResults* parent,
                                std::map<std::string, double>::const_iterator resultIt,
                                std::map<std::string, std::string>::const_iterator errorIt) :
                                parent_(parent), resultIt_(resultIt), errorIt_(errorIt) {
    // ensure all duplicates are skipped to cater for if begin() is already in error phase
    skipDuplicates();
}

void ScalarResults::Iterator::skipDuplicates() {
    if (resultIt_ != parent_->results_.end()) {
        return;
    }
    while (errorIt_ != parent_->errors_.end() && parent_->results_.find(errorIt_->first) != parent_->results_.end()) {
        ++errorIt_;
    }
}
ScalarResults::Iterator& ScalarResults::Iterator::operator++() {
    if (resultIt_ != parent_->results_.end()) {
        ++resultIt_;
        // Crossing from the results phase into the errors phase: errorIt_ is
        // still at errors_.begin() and may point at an already-yielded trade.
        skipDuplicates();
    } else {
        ++errorIt_;
        skipDuplicates();
    }
    return *this;
}

ScalarResult ScalarResults::Iterator::operator*() const {
    // The key identifies the trade; operator[] already merges the result and
    // error for that trade, so the combining logic is not duplicated here.
    const std::string& tradeId = (resultIt_ != parent_->results_.end())
                                     ? resultIt_->first
                                     : errorIt_->first;
    return (*parent_)[tradeId].value();
}

bool ScalarResults::Iterator::operator!=(const Iterator& other) const {
    // Position is defined by both maps, since either may be mid-traversal.
    return resultIt_ != other.resultIt_ || errorIt_ != other.errorIt_;
}

ScalarResults::Iterator ScalarResults::begin() const {
    return Iterator(this, results_.begin(), errors_.begin());
}

ScalarResults::Iterator ScalarResults::end() const {
    // Both maps at end. A default-constructed Iterator would hold singular map
    // iterators, which cannot legally be compared.
    return Iterator(this, results_.end(), errors_.end());
}