#pragma once

#include "status_rollup/status.hpp"
#include <nlohmann/json_fwd.hpp>
#include <memory>
#include <span>
#include <string>

namespace status_rollup {

// Base class for all rollup rules
class RollupRule {
public:
    virtual ~RollupRule() = default;
    virtual Status compute(std::span<const Status> inputs) const = 0;
    virtual std::string name() const = 0;
};

// Rule: Take the worst (highest) status
class WorstStatusRule : public RollupRule {
public:
    Status compute(std::span<const Status> inputs) const override;
    std::string name() const override { return "worst_status"; }
};

// Rule: Require threshold number of reds before rolling up to red
class ThresholdRollupRule : public RollupRule {
public:
    ThresholdRollupRule(int red_threshold, int yellow_to_yellow, int yellow_to_red);
    Status compute(std::span<const Status> inputs) const override;
    std::string name() const override { return "threshold_rollup"; }

private:
    int red_threshold_;
    int yellow_to_yellow_;
    int yellow_to_red_;
};

// Rule: Use majority voting
class MajorityVoteRule : public RollupRule {
public:
    Status compute(std::span<const Status> inputs) const override;
    std::string name() const override { return "majority_vote"; }
};

// Factory for creating rules from JSON
class RuleFactory {
public:
    static std::unique_ptr<RollupRule> create(const std::string& rule_name, const nlohmann::json& params);
};

} // namespace status_rollup
