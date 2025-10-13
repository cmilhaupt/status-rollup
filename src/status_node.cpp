#include "status_node.hpp"

namespace status_rollup {

StatusNode::StatusNode() : name_(""), status_(Status::Unknown) {}

StatusNode::StatusNode(std::string name)
    : name_(std::move(name))
    , status_(Status::Unknown) {}

void StatusNode::set_name(const std::string& name) {
    name_ = name;
}

void StatusNode::set_imported_status(Status status) {
    status_ = status;
}

void StatusNode::set_rule(std::unique_ptr<RollupRule> rule) {
    rule_ = std::move(rule);
}

CStatus StatusNode::run() {
    if (!rule_) {
        // Leaf node - status already set
        return CStatus();
    }

    // Derived node - compute from dependencies
    std::vector<Status> input_statuses;
    for (auto* dep : dependencies_) {
        input_statuses.push_back(static_cast<StatusNode*>(dep)->get_status());
    }

    status_ = rule_->compute(input_statuses);
    return CStatus();
}

Status StatusNode::get_status() const {
    return status_;
}

const std::string& StatusNode::get_name() const {
    return name_;
}

void StatusNode::add_dependency(StatusNode* dep) {
    dependencies_.push_back(dep);
}

const std::vector<StatusNode*>& StatusNode::get_dependencies() const {
    return dependencies_;
}

} // namespace status_rollup
