#include "status_rollup/status_tree.hpp"
#include <iostream>
#include <sstream>
#include <exception>
#include <string>

using namespace status_rollup;

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <config.json>\n";
    std::cerr << "\nInteractive mode:\n";
    std::cerr << "  Enter status updates as: <node_name> <status>\n";
    std::cerr << "  Status values: green, yellow, red\n";
    std::cerr << "  Example: db_primary green\n";
    std::cerr << "  Commands:\n";
    std::cerr << "    print           - Show current tree status\n";
    std::cerr << "    get <node_name> - Get status of specific node\n";
    std::cerr << "    quit            - Exit the program\n";
}

bool is_leaf_node(const StatusTree& tree, const std::string& node_name) {
    // Try to get the status - if it exists, we can check if it's a leaf
    // We'll attempt to set it and catch any errors
    // A better API would expose whether a node is a leaf, but for now
    // we'll use a try-catch approach
    return tree.get_status(node_name).has_value();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        StatusTree tree;

        // Load configuration from command line argument
        std::string config_file = argv[1];
        tree.load_config(config_file);

        // Initialize all leaf nodes to green
        const char* leaf_nodes[] = {
            "db_primary", "db_replica_1", "db_replica_2",
            "cache_node_1", "cache_node_2", "cache_node_3",
            "api_server_1", "api_server_2", "api_server_3",
            "queue_broker_1", "queue_broker_2",
            "auth_service", "payment_service", "notification_service",
            "cdn_edge_1", "cdn_edge_2", "cdn_edge_3", "cdn_edge_4",
            "load_balancer_1", "load_balancer_2",
            "monitoring_prometheus", "monitoring_grafana",
            "logging_elasticsearch", "logging_kibana"
        };

        for (const char* node : leaf_nodes) {
            tree.set_status(node, Status::Green);
        }

        tree.compute();

        std::cout << "Configuration loaded from: " << config_file << "\n";
        std::cout << "All leaf nodes initialized to green\n";
        std::cout << "Enter status updates (format: <node_name> <status>)\n";
        std::cout << "Type 'print' to show tree, 'quit' to exit\n\n";

        std::string line;
        while (true) {
            std::cout << "> ";
            std::cout.flush();

            if (!std::getline(std::cin, line)) {
                // EOF reached
                break;
            }

            // Trim and skip empty lines
            if (line.empty()) {
                continue;
            }

            // Handle commands
            if (line == "quit" || line == "exit") {
                std::cout << "Exiting...\n";
                break;
            }

            if (line == "print" || line == "status") {
                tree.compute();
                tree.print_statuses();

                if (auto status = tree.get_status("overall_system_health")) {
                    std::cout << "\n=========================\n";
                    std::cout << "Overall System Health: "
                              << status_to_string(*status) << "\n";
                    std::cout << "=========================\n\n";
                }
                continue;
            }

            // Handle "get <node_name>" command
            if (line.substr(0, 4) == "get ") {
                std::string node_name = line.substr(4);
                // Trim leading/trailing spaces
                size_t start = node_name.find_first_not_of(" \t");
                size_t end = node_name.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos) {
                    node_name = node_name.substr(start, end - start + 1);
                }

                if (node_name.empty()) {
                    std::cerr << "Error: Usage: get <node_name>\n";
                    continue;
                }

                auto status = tree.get_status(node_name);
                if (status.has_value()) {
                    std::cout << node_name << ": " << status_to_string(*status) << "\n";
                } else {
                    std::cerr << "Error: Node '" << node_name << "' does not exist\n";
                }
                continue;
            }

            // Parse input: <node_name> <status>
            std::istringstream iss(line);
            std::string node_name, status_str;

            if (!(iss >> node_name >> status_str)) {
                std::cerr << "Error: Invalid input format. Expected: <node_name> <status>\n";
                continue;
            }

            // Convert status string to Status enum
            Status status = string_to_status(status_str);
            if (status == Status::Unknown) {
                std::cerr << "Error: Invalid status '" << status_str
                          << "'. Use: green, yellow, or red\n";
                continue;
            }

            // Check if node exists
            if (!tree.get_status(node_name).has_value()) {
                std::cerr << "Error: Node '" << node_name << "' does not exist\n";
                continue;
            }

            // Try to set the status
            try {
                tree.set_status(node_name, status);
                tree.compute();

                std::cout << "Updated " << node_name << " to "
                          << status_to_string(status) << "\n";

                // Show overall health after update
                if (auto overall = tree.get_status("overall_system_health")) {
                    std::cout << "Overall System Health: "
                              << status_to_string(*overall) << "\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what()
                          << " (Note: '" << node_name << "' may be a derived node)\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
