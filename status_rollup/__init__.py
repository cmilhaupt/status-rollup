"""Status Rollup - Hierarchical status monitoring and rollup.

This package provides tools for building and managing hierarchical status trees
with configurable rollup rules for aggregating health status from leaf nodes.
"""

from typing import Optional

try:
    from ._status_rollup import (
        Status,
        StatusTree,
        string_to_status,
        status_to_string,
        __version__,
    )
except ImportError as e:
    raise ImportError(
        "Failed to import the compiled extension module. "
        "Make sure the package was built correctly with: pip install -e ."
    ) from e

# Export public API
__all__ = [
    "Status",
    "StatusTree",
    "string_to_status",
    "status_to_string",
    "__version__",
]

# Add convenience attributes to Status enum
Status.GREEN = Status.GREEN
Status.YELLOW = Status.YELLOW
Status.RED = Status.RED
Status.UNKNOWN = Status.UNKNOWN
