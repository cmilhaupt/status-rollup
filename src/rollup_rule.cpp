#include "rollup_rule.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

namespace status_rollup {

// WorstStatusRule implementation
Status WorstStatusRule::compute(std::span<const Status> inputs) const {
    if (inputs.empty()) return Status::Unknown;

    Status worst = Status::Green;
    for (const auto& status : inputs) {
        if (status > worst) worst = status;
    }
    return worst;
}

// ThresholdRollupRule implementation
ThresholdRollupRule::ThresholdRollupRule(int red_threshold, int yellow_to_yellow, int yellow_to_red)
    : red_threshold_(red_threshold)
    , yellow_to_yellow_(yellow_to_yellow)
    , yellow_to_red_(yellow_to_red) {}

Status ThresholdRollupRule::compute(std::span<const Status> inputs) const {
    if (inputs.empty()) return Status::Unknown;

    int red_count = 0;
    int yellow_count = 0;

    for (const auto& status : inputs) {
        if (status == Status::Red) red_count++;
        else if (status == Status::Yellow) yellow_count++;
    }

    // Check red threshold
    if (red_count >= red_threshold_) return Status::Red;

    // Check yellow thresholds
    if (yellow_count >= yellow_to_red_) return Status::Red;
    if (yellow_count >= yellow_to_yellow_) return Status::Yellow;

    return Status::Green;
}

// MajorityVoteRule implementation
Status MajorityVoteRule::compute(std::span<const Status> inputs) const {
    if (inputs.empty()) return Status::Unknown;

    int counts[3] = {0, 0, 0}; // Green, Yellow, Red

    for (const auto& status : inputs) {
        if (status != Status::Unknown) {
            counts[static_cast<int>(status)]++;
        }
    }

    // Find majority
    int max_count = 0;
    Status majority = Status::Green;
    for (int i = 0; i < 3; ++i) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            majority = static_cast<Status>(i);
        }
    }

    return majority;
}

// RuleFactory implementation
std::unique_ptr<RollupRule> RuleFactory::create(const std::string& rule_name, const json& params) {
    if (rule_name == "worst_status") {
        return std::make_unique<WorstStatusRule>();
    }
    else if (rule_name == "threshold_rollup") {
        int red_threshold = params.value("red_threshold", 1);
        int yellow_to_yellow = params.value("yellow_to_yellow", 1);
        int yellow_to_red = params.value("yellow_to_red", 2);
        return std::make_unique<ThresholdRollupRule>(
            red_threshold, yellow_to_yellow, yellow_to_red
        );
    }
    else if (rule_name == "majority_vote") {
        return std::make_unique<MajorityVoteRule>();
    }

    throw std::runtime_error("Unknown rule: " + rule_name);
}

} // namespace status_rollup
