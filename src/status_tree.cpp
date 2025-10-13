#include "status_rollup/status_tree.hpp"
#include "status_node.hpp"
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

#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace status_rollup {

// Pimpl implementation class
class StatusTree::Impl {
public:
    Impl() {
        pipeline_ = CGraph::GPipelineFactory::create();
    }

    void load_config(const std::string& config_file) {
        std::ifstream file(config_file);
        if (!file) {
            throw std::runtime_error("Cannot open config file: " + config_file);
        }

        json config;
        file >> config;

        // Store node configurations for processing
        std::unordered_map<std::string, json> node_configs;
        for (const auto& [node_name, node_config] : config["nodes"].items()) {
            node_configs[node_name] = node_config;
        }

        // First pass: Create all leaf nodes (no dependencies)
        for (const auto& [node_name, node_config] : node_configs) {
            std::string type = node_config.value("type", "imported");
            if (type == "imported") {
                CGraph::GElementPtr node_ptr = nullptr;
                pipeline_->registerGElement<StatusNode>(&node_ptr, {}, node_name);

                auto* node = dynamic_cast<StatusNode*>(node_ptr);
                if (!node) {
                    throw std::runtime_error("Failed to create node: " + node_name);
                }

                node->set_name(node_name);
                leaf_nodes_.push_back(node);
                nodes_[node_name] = node_ptr;
            }
        }

        // Second pass: Create derived nodes (with dependencies)
        // Keep trying until all nodes are created (handles arbitrary dependency order)
        std::unordered_set<std::string> created;
        for (const auto& [name, _] : nodes_) {
            created.insert(name);
        }

        bool progress = true;
        while (progress) {
            progress = false;

            for (const auto& [node_name, node_config] : node_configs) {
                if (created.count(node_name)) continue;

                std::string type = node_config.value("type", "imported");
                if (type != "derived") continue;

                // Check if all dependencies are created
                std::vector<std::string> dep_names = node_config.value("dependencies", std::vector<std::string>{});
                bool all_deps_ready = true;
                for (const auto& dep_name : dep_names) {
                    if (!created.count(dep_name)) {
                        all_deps_ready = false;
                        break;
                    }
                }

                if (!all_deps_ready) continue;

                // Create node with dependencies
                CGraph::GElementPtrSet deps;
                for (const auto& dep_name : dep_names) {
                    auto it = nodes_.find(dep_name);
                    if (it != nodes_.end()) {
                        deps.insert(it->second);
                    }
                }

                CGraph::GElementPtr node_ptr = nullptr;
                pipeline_->registerGElement<StatusNode>(&node_ptr, deps, node_name);

                auto* node = dynamic_cast<StatusNode*>(node_ptr);
                if (!node) {
                    throw std::runtime_error("Failed to create node: " + node_name);
                }

                node->set_name(node_name);

                // Set up rollup rule
                std::string rule_name = node_config.value("rule", "worst_status");
                json params = node_config.value("params", json::object());
                node->set_rule(RuleFactory::create(rule_name, params));

                // Add dependencies to the node's internal tracking
                for (const auto& dep_name : dep_names) {
                    auto it = nodes_.find(dep_name);
                    if (it != nodes_.end()) {
                        node->add_dependency(dynamic_cast<StatusNode*>(it->second));
                    }
                }

                nodes_[node_name] = node_ptr;
                created.insert(node_name);
                progress = true;
            }
        }

        // Check if all nodes were created
        if (created.size() != node_configs.size()) {
            throw std::runtime_error("Failed to create all nodes - possible circular dependency or missing dependency");
        }
    }

    void set_status(const std::string& node_name, Status status) {
        auto it = nodes_.find(node_name);
        if (it == nodes_.end()) {
            throw std::runtime_error("Unknown node: " + node_name);
        }
        auto* node = dynamic_cast<StatusNode*>(it->second);
        if (node) {
            node->set_imported_status(status);
        }
    }

    void compute() {
        if (pipeline_) {
            pipeline_->process();
        }
    }

    std::optional<Status> get_status(const std::string& node_name) const {
        auto it = nodes_.find(node_name);
        if (it == nodes_.end()) {
            return std::nullopt;
        }
        auto* node = dynamic_cast<StatusNode*>(it->second);
        if (node) {
            return node->get_status();
        }
        return std::nullopt;
    }

    void print_statuses() const {
        std::cout << "Status Tree Results:\n";
        std::cout << "====================\n\n";

        // Separate leaf and derived nodes
        std::vector<std::pair<std::string, StatusNode*>> leaf_list;
        std::vector<std::pair<std::string, StatusNode*>> derived_list;

        for (const auto& [name, node_ptr] : nodes_) {
            auto* node = dynamic_cast<StatusNode*>(node_ptr);
            if (node) {
                if (node->get_dependencies().empty()) {
                    leaf_list.emplace_back(name, node);
                } else {
                    derived_list.emplace_back(name, node);
                }
            }
        }

        // Sort for consistent output
        std::sort(leaf_list.begin(), leaf_list.end());

        // Print leaf nodes
        std::cout << "LEAF NODES (Imported):\n";
        std::cout << "----------------------\n";
        for (const auto& [name, node] : leaf_list) {
            std::cout << "  " << name << ": "
                      << status_to_string(node->get_status()) << "\n";
        }

        // Find root node (node with no dependents in the derived set)
        std::unordered_set<StatusNode*> has_dependents;
        for (const auto& [name, node] : derived_list) {
            for (auto* dep : node->get_dependencies()) {
                has_dependents.insert(dep);
            }
        }

        StatusNode* root = nullptr;
        for (const auto& [name, node] : derived_list) {
            if (has_dependents.find(node) == has_dependents.end()) {
                root = node;
                break;
            }
        }

        // Calculate depth from root using BFS
        std::unordered_map<StatusNode*, int> depths;
        if (root) {
            std::queue<std::pair<StatusNode*, int>> queue;
            queue.push({root, 0});
            depths[root] = 0;

            while (!queue.empty()) {
                auto [current, depth] = queue.front();
                queue.pop();

                for (auto* dep : current->get_dependencies()) {
                    // Only process derived nodes
                    auto* derived_dep = dynamic_cast<StatusNode*>(dep);
                    if (derived_dep && !derived_dep->get_dependencies().empty()) {
                        if (depths.find(derived_dep) == depths.end()) {
                            depths[derived_dep] = depth + 1;
                            queue.push({derived_dep, depth + 1});
                        }
                    }
                }
            }
        }

        // Print in tree order using DFS from root
        std::cout << "\nDERIVED NODES (Computed):\n";
        std::cout << "-------------------------\n";

        std::unordered_set<StatusNode*> visited;
        std::function<void(StatusNode*, int)> print_tree = [&](StatusNode* node, int depth) {
            if (!node || visited.count(node)) {
                return;
            }
            visited.insert(node);

            // Print indentation (tabs based on depth)
            for (int i = 0; i < depth; ++i) {
                std::cout << "\t";
            }

            std::cout << node->get_name() << ": "
                      << status_to_string(node->get_status());

            const auto& deps = node->get_dependencies();
            if (!deps.empty()) {
                std::cout << " <- [";
                for (size_t i = 0; i < deps.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << deps[i]->get_name();
                }
                std::cout << "]";
            }
            std::cout << "\n";

            // Sort dependencies by name for consistent output
            std::vector<StatusNode*> sorted_deps;
            for (auto* dep : deps) {
                auto* derived_dep = dynamic_cast<StatusNode*>(dep);
                if (derived_dep && !derived_dep->get_dependencies().empty()) {
                    sorted_deps.push_back(derived_dep);
                }
            }
            std::sort(sorted_deps.begin(), sorted_deps.end(),
                [](StatusNode* a, StatusNode* b) {
                    return a->get_name() < b->get_name();
                });

            // Recursively print children
            for (auto* dep : sorted_deps) {
                print_tree(dep, depth + 1);
            }
        };

        if (root) {
            print_tree(root, 0);
        }

        std::cout << "\n";
    }

private:
    std::unordered_map<std::string, CGraph::GElementPtr> nodes_;
    std::vector<StatusNode*> leaf_nodes_;
    CGraph::GPipelinePtr pipeline_;
};

// StatusTree public interface implementation
StatusTree::StatusTree() : pimpl_(std::make_unique<Impl>()) {}

StatusTree::~StatusTree() = default;

void StatusTree::load_config(const std::string& config_file) {
    pimpl_->load_config(config_file);
}

void StatusTree::set_status(const std::string& node_name, Status status) {
    pimpl_->set_status(node_name, status);
}

void StatusTree::compute() {
    pimpl_->compute();
}

std::optional<Status> StatusTree::get_status(const std::string& node_name) const {
    return pimpl_->get_status(node_name);
}

void StatusTree::print_statuses() const {
    pimpl_->print_statuses();
}

} // namespace status_rollup
