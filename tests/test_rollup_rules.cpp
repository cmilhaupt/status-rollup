#include "status_rollup/status.hpp"
#include "../src/rollup_rule.hpp"
#include <gtest/gtest.h>
#include <vector>

using namespace status_rollup;

// Test WorstStatusRule
TEST(WorstStatusRuleTest, AllGreen) {
    WorstStatusRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Green, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Green);
}

TEST(WorstStatusRuleTest, OneYellow) {
    WorstStatusRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Yellow, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Yellow);
}

TEST(WorstStatusRuleTest, OneRed) {
    WorstStatusRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Yellow, Status::Red};
    EXPECT_EQ(rule.compute(inputs), Status::Red);
}

TEST(WorstStatusRuleTest, EmptyInput) {
    WorstStatusRule rule;
    std::vector<Status> inputs;
    EXPECT_EQ(rule.compute(inputs), Status::Unknown);
}

// Test ThresholdRollupRule
TEST(ThresholdRollupRuleTest, BelowRedThreshold) {
    ThresholdRollupRule rule(2, 1, 3); // red_threshold=2, yellow_to_yellow=1, yellow_to_red=3
    std::vector<Status> inputs = {Status::Red, Status::Green, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Green);
}

TEST(ThresholdRollupRuleTest, AtRedThreshold) {
    ThresholdRollupRule rule(2, 1, 3);
    std::vector<Status> inputs = {Status::Red, Status::Red, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Red);
}

TEST(ThresholdRollupRuleTest, YellowToYellow) {
    ThresholdRollupRule rule(2, 1, 3);
    std::vector<Status> inputs = {Status::Yellow, Status::Green, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Yellow);
}

TEST(ThresholdRollupRuleTest, YellowToRed) {
    ThresholdRollupRule rule(2, 1, 3);
    std::vector<Status> inputs = {Status::Yellow, Status::Yellow, Status::Yellow};
    EXPECT_EQ(rule.compute(inputs), Status::Red);
}

TEST(ThresholdRollupRuleTest, AllGreen) {
    ThresholdRollupRule rule(2, 1, 3);
    std::vector<Status> inputs = {Status::Green, Status::Green, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Green);
}

// Test MajorityVoteRule
TEST(MajorityVoteRuleTest, GreenMajority) {
    MajorityVoteRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Green, Status::Yellow};
    EXPECT_EQ(rule.compute(inputs), Status::Green);
}

TEST(MajorityVoteRuleTest, YellowMajority) {
    MajorityVoteRule rule;
    std::vector<Status> inputs = {Status::Yellow, Status::Yellow, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Yellow);
}

TEST(MajorityVoteRuleTest, RedMajority) {
    MajorityVoteRule rule;
    std::vector<Status> inputs = {Status::Red, Status::Red, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Red);
}

TEST(MajorityVoteRuleTest, Tie) {
    MajorityVoteRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Yellow};
    // Should return the first one found with max count (Green)
    Status result = rule.compute(inputs);
    EXPECT_TRUE(result == Status::Green || result == Status::Yellow);
}

TEST(MajorityVoteRuleTest, WithUnknown) {
    MajorityVoteRule rule;
    std::vector<Status> inputs = {Status::Green, Status::Unknown, Status::Green};
    EXPECT_EQ(rule.compute(inputs), Status::Green);
}

// Test Status conversion functions
TEST(StatusConversionTest, StringToStatus) {
    EXPECT_EQ(string_to_status("green"), Status::Green);
    EXPECT_EQ(string_to_status("yellow"), Status::Yellow);
    EXPECT_EQ(string_to_status("red"), Status::Red);
    EXPECT_EQ(string_to_status("invalid"), Status::Unknown);
}

TEST(StatusConversionTest, StatusToString) {
    EXPECT_EQ(status_to_string(Status::Green), "green");
    EXPECT_EQ(status_to_string(Status::Yellow), "yellow");
    EXPECT_EQ(status_to_string(Status::Red), "red");
    EXPECT_EQ(status_to_string(Status::Unknown), "unknown");
}
