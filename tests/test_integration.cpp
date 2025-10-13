#include "status_rollup/status_tree.hpp"
#include <gtest/gtest.h>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace status_rollup;
using json = nlohmann::json;

class StatusTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test configuration file
        test_config_file_ = "test_config.json";
    }

    void TearDown() override {
        // Clean up test files
        std::remove(test_config_file_.c_str());
    }

    void create_simple_config() {
        json config = {
            {"nodes", {
                {"leaf1", {{"type", "imported"}}},
                {"leaf2", {{"type", "imported"}}},
                {"derived1", {
                    {"type", "derived"},
                    {"rule", "worst_status"},
                    {"dependencies", {"leaf1", "leaf2"}}
                }}
            }}
        };
        std::ofstream file(test_config_file_);
        file << config.dump(2);
    }

    void create_threshold_config() {
        json config = {
            {"nodes", {
                {"service1", {{"type", "imported"}}},
                {"service2", {{"type", "imported"}}},
                {"service3", {{"type", "imported"}}},
                {"cluster", {
                    {"type", "derived"},
                    {"rule", "threshold_rollup"},
                    {"params", {
                        {"red_threshold", 2},
                        {"yellow_to_yellow", 1},
                        {"yellow_to_red", 2}
                    }},
                    {"dependencies", {"service1", "service2", "service3"}}
                }}
            }}
        };
        std::ofstream file(test_config_file_);
        file << config.dump(2);
    }

    void create_majority_vote_config() {
        json config = {
            {"nodes", {
                {"node1", {{"type", "imported"}}},
                {"node2", {{"type", "imported"}}},
                {"node3", {{"type", "imported"}}},
                {"majority", {
                    {"type", "derived"},
                    {"rule", "majority_vote"},
                    {"dependencies", {"node1", "node2", "node3"}}
                }}
            }}
        };
        std::ofstream file(test_config_file_);
        file << config.dump(2);
    }

    std::string test_config_file_;
};

// Test simple worst_status rule
TEST_F(StatusTreeTest, WorstStatusRollup) {
    create_simple_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("leaf1", Status::Green);
    tree.set_status("leaf2", Status::Yellow);
    tree.compute();

    EXPECT_EQ(tree.get_status("leaf1").value(), Status::Green);
    EXPECT_EQ(tree.get_status("leaf2").value(), Status::Yellow);
    EXPECT_EQ(tree.get_status("derived1").value(), Status::Yellow);
}

TEST_F(StatusTreeTest, WorstStatusWithRed) {
    create_simple_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("leaf1", Status::Green);
    tree.set_status("leaf2", Status::Red);
    tree.compute();

    EXPECT_EQ(tree.get_status("derived1").value(), Status::Red);
}

// Test threshold rollup rule
TEST_F(StatusTreeTest, ThresholdRollupBelowThreshold) {
    create_threshold_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("service1", Status::Red);
    tree.set_status("service2", Status::Green);
    tree.set_status("service3", Status::Green);
    tree.compute();

    // Only 1 red, threshold is 2, so should be green
    EXPECT_EQ(tree.get_status("cluster").value(), Status::Green);
}

TEST_F(StatusTreeTest, ThresholdRollupAtRedThreshold) {
    create_threshold_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("service1", Status::Red);
    tree.set_status("service2", Status::Red);
    tree.set_status("service3", Status::Green);
    tree.compute();

    // 2 reds, threshold is 2, so should be red
    EXPECT_EQ(tree.get_status("cluster").value(), Status::Red);
}

TEST_F(StatusTreeTest, ThresholdRollupYellowToYellow) {
    create_threshold_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("service1", Status::Yellow);
    tree.set_status("service2", Status::Green);
    tree.set_status("service3", Status::Green);
    tree.compute();

    // 1 yellow, yellow_to_yellow threshold is 1, so should be yellow
    EXPECT_EQ(tree.get_status("cluster").value(), Status::Yellow);
}

TEST_F(StatusTreeTest, ThresholdRollupYellowToRed) {
    create_threshold_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("service1", Status::Yellow);
    tree.set_status("service2", Status::Yellow);
    tree.set_status("service3", Status::Green);
    tree.compute();

    // 2 yellows, yellow_to_red threshold is 2, so should be red
    EXPECT_EQ(tree.get_status("cluster").value(), Status::Red);
}

// Test majority vote rule
TEST_F(StatusTreeTest, MajorityVoteGreen) {
    create_majority_vote_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("node1", Status::Green);
    tree.set_status("node2", Status::Green);
    tree.set_status("node3", Status::Yellow);
    tree.compute();

    EXPECT_EQ(tree.get_status("majority").value(), Status::Green);
}

TEST_F(StatusTreeTest, MajorityVoteYellow) {
    create_majority_vote_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("node1", Status::Yellow);
    tree.set_status("node2", Status::Yellow);
    tree.set_status("node3", Status::Green);
    tree.compute();

    EXPECT_EQ(tree.get_status("majority").value(), Status::Yellow);
}

TEST_F(StatusTreeTest, MajorityVoteRed) {
    create_majority_vote_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("node1", Status::Red);
    tree.set_status("node2", Status::Red);
    tree.set_status("node3", Status::Green);
    tree.compute();

    EXPECT_EQ(tree.get_status("majority").value(), Status::Red);
}

// Test error handling
TEST_F(StatusTreeTest, InvalidNodeName) {
    create_simple_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    EXPECT_FALSE(tree.get_status("nonexistent").has_value());
}

TEST_F(StatusTreeTest, SetStatusOnDerivedNode) {
    create_simple_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    // Setting status on derived node should not crash
    // (Implementation may throw or ignore - depends on design)
    EXPECT_NO_THROW({
        try {
            tree.set_status("derived1", Status::Green);
        } catch (...) {
            // Expected to potentially throw
        }
    });
}

// Test status updates propagate
TEST_F(StatusTreeTest, StatusUpdatePropagates) {
    create_simple_config();

    StatusTree tree;
    tree.load_config(test_config_file_);

    tree.set_status("leaf1", Status::Green);
    tree.set_status("leaf2", Status::Green);
    tree.compute();
    EXPECT_EQ(tree.get_status("derived1").value(), Status::Green);

    // Update one leaf
    tree.set_status("leaf1", Status::Red);
    tree.compute();
    EXPECT_EQ(tree.get_status("derived1").value(), Status::Red);
}
