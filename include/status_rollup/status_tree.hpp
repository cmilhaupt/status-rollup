#pragma once

#include "status.hpp"
#include <memory>
#include <optional>
#include <string>

namespace CGraph {
    class GPipeline;
    class GElement;
}

namespace status_rollup {

class StatusNode;

// Main status tree manager - public API
class StatusTree {
public:
    StatusTree();
    ~StatusTree();

    // Non-copyable, non-movable (contains unique CGraph pipeline)
    StatusTree(const StatusTree&) = delete;
    StatusTree& operator=(const StatusTree&) = delete;
    StatusTree(StatusTree&&) = delete;
    StatusTree& operator=(StatusTree&&) = delete;

    // Load configuration from JSON file
    void load_config(const std::string& config_file);

    // Import status for a leaf node
    void set_status(const std::string& node_name, Status status);

    // Compute all derived statuses
    void compute();

    // Get status of any node
    std::optional<Status> get_status(const std::string& node_name) const;

    // Print all node statuses with tree structure
    void print_statuses() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace status_rollup
