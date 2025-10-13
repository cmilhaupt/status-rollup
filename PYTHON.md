# Python Package Guide

This document provides detailed instructions for installing and using the `status-rollup` Python package.

## Installation

### Prerequisites

- Python 3.8 or later
- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- CMake 3.14 or later (will be installed automatically by scikit-build-core if not present)
- Internet connection (for downloading C++ dependencies during build)

### Install from Source

#### Using pip

```bash
# Development installation (editable mode)
pip install -e .

# Regular installation
pip install .

# With verbose output to see build progress
pip install -v .
```

#### Using uv (fast)

```bash
# Development installation
uv pip install -e .

# Regular installation
uv pip install .
```

#### Using poetry

```bash
# Add to existing poetry project
poetry add --editable .

# Or in pyproject.toml
[tool.poetry.dependencies]
status-rollup = {path = ".", develop = true}
```

### What Happens During Installation

The installation process:

1. **Downloads C++ dependencies** via CMake FetchContent:
   - CGraph v3.1.2 (DAG computation framework)
   - nlohmann/json v3.12.0 (JSON parsing)
   - pybind11 v2.12.0 (Python bindings)

2. **Compiles the C++ library** (`status_rollup_lib`)

3. **Builds Python bindings** using pybind11

4. **Installs the Python package** with:
   - `status_rollup` module (Python code + compiled extension)
   - Type stubs for IDE support
   - PEP 561 marker for type checkers

### Verifying Installation

```bash
# Check that the package is installed
python -c "import status_rollup; print(status_rollup.__version__)"

# Should output: 0.1.0
```

## Quick Start

### Basic Example

```python
from status_rollup import Status, StatusTree, status_to_string

# Create and load configuration
tree = StatusTree()
tree.load_config("config.json")

# Set leaf node statuses
tree.set_status("service_a", Status.GREEN)
tree.set_status("service_b", Status.RED)
tree.set_status("service_c", Status.YELLOW)

# Compute derived nodes
tree.compute()

# Query results
overall = tree.get_status("overall_health")
if overall is not None:
    print(f"Overall Health: {status_to_string(overall)}")

# Print tree visualization
tree.print_statuses()
```

### Interactive Application

Run the included Python example:

```bash
python examples/python_example.py complex_status_config.json
```

Commands:
- `<node_name> <status>` - Update a leaf node's status
- `get <node_name>` - Query any node's status
- `print` - Display the entire tree
- `quit` - Exit

Example session:

```
> db_primary red
Updated db_primary to red
Overall System Health: red

> get microservices
microservices: red

> print
LEAF NODES (imported):
db_primary: red
...

> quit
Exiting...
```

## API Reference

### Status Enum

```python
from status_rollup import Status

Status.GREEN    # 0 - Healthy/Normal
Status.YELLOW   # 1 - Warning/Degraded
Status.RED      # 2 - Critical/Failed
Status.UNKNOWN  # 3 - Not yet computed or error
```

The Status enum is compatible with Python's `IntEnum`:

```python
# Can compare with integers
assert Status.GREEN == 0
assert Status.RED > Status.YELLOW

# Can convert to/from strings
from status_rollup import string_to_status, status_to_string

status = string_to_status("red")  # Status.RED
name = status_to_string(Status.GREEN)  # "green"
```

### StatusTree Class

```python
class StatusTree:
    """Hierarchical status tree with configurable rollup rules."""

    def __init__(self) -> None:
        """Create a new StatusTree."""

    def load_config(self, config_file: str) -> None:
        """Load tree configuration from a JSON file.

        Args:
            config_file: Path to JSON configuration file

        Raises:
            RuntimeError: If configuration is invalid or file cannot be read
        """

    def set_status(self, node_name: str, status: Status) -> None:
        """Set the status of a leaf node.

        Args:
            node_name: Name of the leaf node to update
            status: New status value

        Raises:
            RuntimeError: If node doesn't exist or is not a leaf node
        """

    def compute(self) -> None:
        """Compute all derived node statuses based on rollup rules.

        This propagates status values from leaf nodes through the dependency
        graph. Call this after updating leaf node statuses.
        """

    def get_status(self, node_name: str) -> Optional[Status]:
        """Get the status of any node.

        Args:
            node_name: Name of the node to query

        Returns:
            Status of the node, or None if node doesn't exist
        """

    def print_statuses(self) -> None:
        """Print hierarchical tree visualization to stdout."""
```

### Conversion Functions

```python
def string_to_status(s: str) -> Status:
    """Convert string to Status enum.

    Args:
        s: Status string (case-insensitive): "green", "yellow", "red", "unknown"

    Returns:
        Corresponding Status enum value

    Raises:
        ValueError: If string is not a valid status value
    """

def status_to_string(status: Status) -> str:
    """Convert Status enum to string representation.

    Args:
        status: Status enum value

    Returns:
        String representation: "green", "yellow", "red", or "unknown"
    """
```

## Type Checking

The package includes comprehensive type stubs for static type checking:

### With mypy

```bash
# Install mypy
pip install mypy

# Run type checker
mypy your_script.py
```

### With pyright/pylance

Type stubs are automatically recognized by VS Code with Pylance extension or pyright:

```python
from status_rollup import StatusTree, Status

tree = StatusTree()
tree.load_config("config.json")  # ✓ Type checked
tree.set_status("node", Status.GREEN)  # ✓ Type checked

# Type errors will be caught:
tree.set_status("node", "green")  # ✗ Error: Expected Status, got str
tree.compute(123)  # ✗ Error: compute() takes no arguments
```

## Common Patterns

### Monitoring Multiple Services

```python
from status_rollup import Status, StatusTree

def monitor_services(services_status: dict[str, str]) -> Status:
    """Monitor a collection of services and return overall health.

    Args:
        services_status: Map of service name to status string

    Returns:
        Overall system health status
    """
    tree = StatusTree()
    tree.load_config("monitoring_config.json")

    # Update all service statuses
    for service, status_str in services_status.items():
        status = string_to_status(status_str)
        tree.set_status(service, status)

    # Compute and return overall health
    tree.compute()
    overall = tree.get_status("overall_health")
    return overall if overall is not None else Status.UNKNOWN
```

### Integration with FastAPI

```python
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from status_rollup import Status, StatusTree, status_to_string

app = FastAPI()
tree = StatusTree()
tree.load_config("api_services_config.json")

class StatusUpdate(BaseModel):
    node: str
    status: str

@app.post("/status/{node}")
async def update_status(node: str, status: str):
    """Update a service status."""
    try:
        tree.set_status(node, string_to_status(status))
        tree.compute()

        overall = tree.get_status("overall_health")
        return {
            "node": node,
            "status": status,
            "overall_health": status_to_string(overall) if overall else "unknown"
        }
    except RuntimeError as e:
        raise HTTPException(status_code=400, detail=str(e))

@app.get("/status/{node}")
async def get_status(node: str):
    """Query a node's status."""
    status = tree.get_status(node)
    if status is None:
        raise HTTPException(status_code=404, detail=f"Node '{node}' not found")
    return {"node": node, "status": status_to_string(status)}
```

### Scheduled Health Checks

```python
import time
from status_rollup import Status, StatusTree, status_to_string

def health_check_loop(tree: StatusTree, check_interval: int = 60):
    """Periodically check and update service health.

    Args:
        tree: Configured StatusTree instance
        check_interval: Seconds between checks
    """
    while True:
        # Check each service (pseudo-code)
        for service in get_monitored_services():
            health = check_service_health(service)
            tree.set_status(service, health)

        # Compute overall status
        tree.compute()
        overall = tree.get_status("overall_health")

        # Log or alert based on status
        if overall == Status.RED:
            send_alert(f"System critical: {status_to_string(overall)}")

        time.sleep(check_interval)
```

## Troubleshooting

### Build Failures

**Problem:** Build fails with "C++ compiler not found"

**Solution:** Install a C++20 compiler:
- **Linux:** `sudo apt install g++` or `sudo yum install gcc-c++`
- **macOS:** `xcode-select --install`
- **Windows:** Install Visual Studio 2019+ or MinGW-w64

---

**Problem:** Build fails with "CMake not found"

**Solution:** scikit-build-core should install CMake automatically. If not:
```bash
pip install cmake
```

---

**Problem:** Build fails downloading dependencies

**Solution:** Check internet connection. Dependencies are downloaded from GitHub. If behind a proxy, configure Git:
```bash
git config --global http.proxy http://proxy:port
```

### Runtime Errors

**Problem:** `ImportError: Failed to import the compiled extension module`

**Solution:** The C++ extension wasn't built. Try reinstalling:
```bash
pip uninstall status-rollup
pip install -v .  # Verbose to see build output
```

---

**Problem:** `RuntimeError: Node 'X' does not exist`

**Solution:** Check that:
1. The node name is spelled correctly (case-sensitive)
2. The node is defined in your configuration file
3. `load_config()` was called successfully

---

**Problem:** All derived nodes show `UNKNOWN` status

**Solution:** Call `tree.compute()` after setting leaf node statuses.

## Development

### Building in Development Mode

For faster iteration during development:

```bash
# Install in editable mode
pip install -e .

# Make changes to Python code - no rebuild needed
# Changes to C++ code require rebuild:
pip install -e . --force-reinstall --no-deps
```

### Running Tests

```bash
# Python structure tests (no build required)
python tests/test_python_structure.py

# C++ tests (requires building C++ library first)
cmake -Bbuild
cmake --build build -j
cd build && ctest
```

## Performance Notes

- **Tree computation** uses CGraph's thread pool for parallel execution
- **Large trees** (100+ nodes) benefit from the parallel computation
- **Memory usage** is proportional to tree size (typically <1MB for 100 nodes)
- **Computation time** is typically <1ms for trees with <50 nodes on modern hardware

## See Also

- [README.md](README.md) - Main project documentation
- [CONFIG_FORMAT.md](CONFIG_FORMAT.md) - JSON configuration reference
- [examples/python_example.py](examples/python_example.py) - Complete Python example
