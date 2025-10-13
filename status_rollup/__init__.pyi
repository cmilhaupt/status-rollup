"""Type stubs for status_rollup."""

from enum import IntEnum
from typing import Optional

__version__: str

class Status(IntEnum):
    """Status enumeration for health monitoring.

    Attributes:
        GREEN: Healthy/Normal status (0)
        YELLOW: Warning/Degraded status (1)
        RED: Critical/Failed status (2)
        UNKNOWN: Unknown/Uncomputed status (3)
    """
    GREEN: int
    YELLOW: int
    RED: int
    UNKNOWN: int

class StatusTree:
    """Hierarchical status tree with configurable rollup rules.

    A StatusTree manages a directed acyclic graph (DAG) of status nodes where:
    - Leaf nodes (imported) have their status set externally
    - Derived nodes compute their status from dependencies using rollup rules

    Example:
        >>> tree = StatusTree()
        >>> tree.load_config("config.json")
        >>> tree.set_status("service_a", Status.GREEN)
        >>> tree.set_status("service_b", Status.RED)
        >>> tree.compute()
        >>> status = tree.get_status("overall_health")
        >>> print(status_to_string(status))
        red
    """

    def __init__(self) -> None:
        """Create a new StatusTree."""
        ...

    def load_config(self, config_file: str) -> None:
        """Load tree configuration from a JSON file.

        The configuration file defines the node hierarchy, dependencies,
        and rollup rules. See CONFIG_FORMAT.md for details.

        Args:
            config_file: Path to JSON configuration file

        Raises:
            RuntimeError: If configuration is invalid or file cannot be read
        """
        ...

    def set_status(self, node_name: str, status: Status) -> None:
        """Set the status of a leaf node.

        This updates a leaf (imported) node's status. After setting statuses,
        call compute() to propagate changes through the tree.

        Args:
            node_name: Name of the leaf node to update
            status: New status value

        Raises:
            RuntimeError: If node doesn't exist or is not a leaf node
        """
        ...

    def compute(self) -> None:
        """Compute all derived node statuses based on rollup rules.

        This propagates status values from leaf nodes through the dependency
        graph, computing derived nodes according to their rollup rules.
        Call this after updating leaf node statuses.
        """
        ...

    def get_status(self, node_name: str) -> Optional[Status]:
        """Get the status of any node.

        Args:
            node_name: Name of the node to query

        Returns:
            Status of the node, or None if node doesn't exist
        """
        ...

    def print_statuses(self) -> None:
        """Print hierarchical tree visualization to stdout.

        Displays leaf nodes and derived nodes with their current status values,
        with derived nodes indented according to their depth in the tree.
        """
        ...

def string_to_status(s: str) -> Status:
    """Convert string to Status enum.

    Args:
        s: Status string (case-insensitive): "green", "yellow", "red", "unknown"

    Returns:
        Corresponding Status enum value

    Raises:
        ValueError: If string is not a valid status value
    """
    ...

def status_to_string(status: Status) -> str:
    """Convert Status enum to string representation.

    Args:
        status: Status enum value

    Returns:
        String representation: "green", "yellow", "red", or "unknown"
    """
    ...
