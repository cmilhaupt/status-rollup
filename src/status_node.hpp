#pragma once

#include "status_rollup/status.hpp"
#include "rollup_rule.hpp"

// Suppress warnings from CGraph headers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wunused-variable"
#include <CGraph.h>
#pragma GCC diagnostic pop

#include <memory>
#include <string>
#include <vector>

namespace status_rollup {

// Node in the status tree
class StatusNode : public CGraph::GNode {
public:
    StatusNode();
    explicit StatusNode(std::string name);

    void set_name(const std::string& name);

    // For leaf nodes: set imported status
    void set_imported_status(Status status);

    // For derived nodes: set rollup rule
    void set_rule(std::unique_ptr<RollupRule> rule);

    // CGraph execution override
    CStatus run() override;

    Status get_status() const;
    const std::string& get_name() const;

    void add_dependency(StatusNode* dep);
    const std::vector<StatusNode*>& get_dependencies() const;

private:
    std::string name_;
    Status status_;
    std::unique_ptr<RollupRule> rule_;
    std::vector<StatusNode*> dependencies_;
};

} // namespace status_rollup
