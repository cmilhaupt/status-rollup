#!/usr/bin/env python3
"""Example usage of the status_rollup Python package."""

import sys
from pathlib import Path

from status_rollup import Status, StatusTree, status_to_string


def main() -> int:
    """Run the Python example."""
    # Check for config file argument
    if len(sys.argv) < 2:
        print("Usage: python_example.py <config_file.json>")
        return 1

    config_file = sys.argv[1]

    # Verify config file exists
    if not Path(config_file).exists():
        print(f"Error: Config file not found: {config_file}")
        return 1

    # Create and configure the tree
    tree = StatusTree()
    tree.load_config(config_file)
    print(f"Configuration loaded from: {config_file}")

    # Initialize all leaf nodes to green
    leaf_nodes = [
        "db_primary", "db_replica_1", "db_replica_2",
        "cache_node_1", "cache_node_2", "cache_node_3",
        "api_server_1", "api_server_2", "api_server_3",
        "auth_service", "user_service", "payment_service",
        "order_service", "inventory_service",
        "queue_broker_1", "queue_broker_2",
        "cdn_edge_1", "cdn_edge_2", "cdn_edge_3",
        "log_server", "metrics_server",
    ]

    for node in leaf_nodes:
        tree.set_status(node, Status.GREEN)

    tree.compute()
    print("All leaf nodes initialized to green\n")

    # Interactive mode
    print("Enter status updates (format: <node_name> <status>)")
    print("Type 'print' to show tree, 'get <node_name>' to query, 'quit' to exit\n")

    while True:
        try:
            line = input("> ").strip()
            if not line:
                continue

            parts = line.split()
            command = parts[0].lower()

            if command == "quit":
                print("Exiting...")
                break

            elif command == "print":
                tree.print_statuses()

            elif command == "get":
                if len(parts) < 2:
                    print("Usage: get <node_name>")
                    continue

                node_name = parts[1]
                status = tree.get_status(node_name)
                if status is not None:
                    print(f"{node_name}: {status_to_string(status)}")
                else:
                    print(f"Error: Node '{node_name}' does not exist")

            else:
                # Assume it's a status update: <node_name> <status>
                if len(parts) < 2:
                    print("Invalid command. Use: <node_name> <status>, 'print', "
                          "'get <node_name>', or 'quit'")
                    continue

                node_name = parts[0]
                status_str = parts[1].lower()

                # Parse status
                status_map = {
                    "green": Status.GREEN,
                    "yellow": Status.YELLOW,
                    "red": Status.RED,
                    "unknown": Status.UNKNOWN,
                }

                if status_str not in status_map:
                    print(f"Invalid status: {status_str}. "
                          f"Use: green, yellow, red, or unknown")
                    continue

                status = status_map[status_str]

                # Update status
                try:
                    tree.set_status(node_name, status)
                    tree.compute()

                    # Show overall health
                    overall = tree.get_status("overall_system_health")
                    if overall is not None:
                        print(f"Updated {node_name} to {status_str}")
                        print(f"Overall System Health: {status_to_string(overall)}")
                    else:
                        print(f"Updated {node_name} to {status_str}")
                except RuntimeError as e:
                    print(f"Error: {e}")

        except EOFError:
            print("\nExiting...")
            break
        except KeyboardInterrupt:
            print("\nExiting...")
            break

    return 0


if __name__ == "__main__":
    sys.exit(main())
