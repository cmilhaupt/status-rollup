# Status Rollup Tree

[![CI](https://github.com/cmilhaupt/status-rollup/actions/workflows/ci.yml/badge.svg)](https://github.com/cmilhaupt/status-rollup/actions/workflows/ci.yml)
[![PyPI version](https://badge.fury.io/py/status-rollup.svg)](https://badge.fury.io/py/status-rollup)
[![Python versions](https://img.shields.io/pypi/pyversions/status-rollup.svg)](https://pypi.org/project/status-rollup/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A C++20 library and interactive application for hierarchical status monitoring and rollup. Define complex status trees with configurable rollup rules to aggregate health status from leaf nodes up through intermediate layers to an overall system health metric.

## Features

- **Hierarchical Status Trees**: Define multi-level dependency trees with leaf nodes (imported statuses) and derived nodes (computed from dependencies)
- **Multiple Rollup Rules**:
  - `worst_status`: Propagates the worst status among dependencies
  - `threshold_rollup`: Flexible thresholds for red/yellow transitions based on dependency counts
  - `majority_vote`: Status determined by majority voting among dependencies
- **Interactive Application**: Real-time status updates and tree visualization
- **Clean API**: Pimpl pattern hides implementation details and third-party dependencies
- **Comprehensive Testing**: Unit tests, integration tests, and end-to-end shell script tests

## Quick Start

### C++ Library

#### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 11+)
- CMake 3.14 or later
- Internet connection (for downloading dependencies during first build)

#### Building

```bash
# Configure and build
cmake -Bbuild
cmake --build build -j

# Run tests
cd build && ctest --output-on-failure
```

#### CMake Configuration Options

The project supports several CMake options to customize the build:

```bash
# Build without Python bindings (enabled by default)
cmake -Bbuild -DBUILD_PYTHON_BINDINGS=OFF

# Build without tests (tests enabled by default)
cmake -Bbuild -DBUILD_TESTS=OFF

# Build in Release mode (default is Debug when building standalone)
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release

# Combine multiple options
cmake -Bbuild \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=OFF
```

**Available Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTS` | `ON` | Build unit tests, integration tests, and shell script tests |
| `BUILD_PYTHON_BINDINGS` | `ON` | Build Python bindings with pybind11 (requires Python dev headers) |
| `CMAKE_BUILD_TYPE` | Varies | Build type: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel` |

**Notes:**
- Python bindings are enabled by default and include pytest tests if pytest is installed
- Tests download GoogleTest (v1.14.0) during configuration
- Python bindings require pybind11 and Python development headers
- All C++ dependencies (CGraph, nlohmann/json) are downloaded automatically via CMake FetchContent

#### Running the C++ Example

The `rollup` executable provides an interactive interface for monitoring a status tree:

```bash
# Run with the example configuration
./build/rollup complex_status_config.json
```

### Python Package

#### Installation

Install directly from source using pip (works with pip, poetry, uv, etc.):

```bash
# Install in development mode
pip install -e .

# Or install normally
pip install .

# With uv
uv pip install -e .

# With poetry
poetry add -e .
```

The package will automatically:
- Download and build dependencies (CGraph, nlohmann/json, pybind11)
- Compile the C++ library
- Build Python bindings
- Install the `status_rollup` Python package

#### Python Usage

```python
from status_rollup import Status, StatusTree, status_to_string

# Create and configure the tree
tree = StatusTree()
tree.load_config("complex_status_config.json")

# Set leaf node statuses
tree.set_status("db_primary", Status.GREEN)
tree.set_status("payment_service", Status.RED)

# Compute derived statuses
tree.compute()

# Query status
overall = tree.get_status("overall_system_health")
if overall is not None:
    print(f"Overall: {status_to_string(overall)}")

# Print tree visualization
tree.print_statuses()
```

#### Running the Python Example

```bash
# Run the interactive Python example
python examples/python_example.py complex_status_config.json

# Or if installed
python -m status_rollup.examples.python_example complex_status_config.json
```

The Python example provides the same interactive interface as the C++ version.

### Interactive Commands

Once running (C++ or Python), you can interact with the status tree:

- **Set status**: `<node_name> <status>`
  ```
  > db_primary red
  Updated db_primary to red
  Overall System Health: red
  ```

- **Query status**: `get <node_name>`
  ```
  > get overall_system_health
  overall_system_health: red
  ```

- **Print tree**: `print`
  ```
  > print
  LEAF NODES (imported):
  db_primary: red
  db_replica_1: green
  ...

  DERIVED NODES (computed):
  overall_system_health: red
      platform_delivery: green
          api_cluster: green
  ...
  ```

- **Exit**: `quit`

### Example Session

```bash
$ ./build/rollup complex_status_config.json
Configuration loaded from: complex_status_config.json
All leaf nodes initialized to green
Enter status updates (format: <node_name> <status>)
Type 'print' to show tree, 'quit' to exit

> payment_service red
Updated payment_service to red
Overall System Health: red

> get microservices
microservices: red

> db_replica_2 yellow
Updated db_replica_2 to yellow
Overall System Health: red

> print
[... tree visualization ...]

> quit
Exiting...
```

## Project Structure

```
status-rollup/
├── CMakeLists.txt              # Build configuration
├── pyproject.toml              # Python package configuration
├── README.md                   # This file
├── CONFIG_FORMAT.md            # JSON configuration reference
├── complex_status_config.json  # Example 40-node configuration
├── include/
│   └── status_rollup/
│       ├── status.hpp          # Status enum and conversion functions
│       └── status_tree.hpp     # StatusTree public API
├── src/
│   ├── rollup_rule.hpp/cpp     # Rollup rule implementations
│   ├── status_node.hpp/cpp     # Internal node implementation
│   └── status_tree.cpp         # StatusTree implementation
├── python/
│   └── status_rollup_py.cpp    # Python bindings (pybind11)
├── status_rollup/
│   ├── __init__.py             # Python package interface
│   ├── __init__.pyi            # Type stubs for IDE support
│   └── py.typed                # PEP 561 marker for type checking
├── examples/
│   ├── basic_example.cpp       # C++ interactive application
│   └── python_example.py       # Python interactive application
└── tests/
    ├── test_rollup_rules.cpp   # Unit tests (16 tests)
    ├── test_integration.cpp    # Integration tests (12 tests)
    └── test_interactive.sh     # Shell script tests (10 tests)
```

## Using the Library

### C++ API

#### Basic Usage

```cpp
#include <status_rollup/status_tree.hpp>
#include <status_rollup/status.hpp>

using namespace status_rollup;

int main() {
    // Create and configure the tree
    StatusTree tree;
    tree.load_config("my_config.json");

    // Update leaf node statuses
    tree.set_status("service_a", Status::Green);
    tree.set_status("service_b", Status::Red);

    // Compute rollup
    tree.compute();

    // Query results
    auto overall = tree.get_status("overall_health");
    if (overall) {
        std::cout << "Overall: " << status_to_string(*overall) << "\n";
    }

    // Visualize the tree
    tree.print_statuses();

    return 0;
}
```

#### Status Values

The library supports four status levels:
- `Status::Green` - Healthy/Normal
- `Status::Yellow` - Warning/Degraded
- `Status::Red` - Critical/Failed
- `Status::Unknown` - Not yet computed or error

#### C++ API Reference

##### `StatusTree` Class

- `StatusTree()` - Constructor
- `~StatusTree()` - Destructor
- `void load_config(const std::string& config_file)` - Load tree configuration from JSON
- `void set_status(const std::string& node_name, Status status)` - Update a leaf node's status
- `void compute()` - Compute all derived node statuses based on rollup rules
- `std::optional<Status> get_status(const std::string& node_name) const` - Query any node's status
- `void print_statuses() const` - Print hierarchical tree visualization

##### Status Conversion

- `constexpr Status string_to_status(std::string_view s)` - Convert string to Status enum
- `constexpr std::string_view status_to_string(Status s)` - Convert Status enum to string

Supported string values: `"green"`, `"yellow"`, `"red"`, `"unknown"` (case-insensitive)

### Python API

The Python API mirrors the C++ API with Pythonic conventions:

```python
from status_rollup import Status, StatusTree, string_to_status, status_to_string

# Status enum values
Status.GREEN    # 0
Status.YELLOW   # 1
Status.RED      # 2
Status.UNKNOWN  # 3

# StatusTree class
tree = StatusTree()
tree.load_config(config_file: str) -> None
tree.set_status(node_name: str, status: Status) -> None
tree.compute() -> None
tree.get_status(node_name: str) -> Optional[Status]
tree.print_statuses() -> None

# Conversion functions
string_to_status(s: str) -> Status
status_to_string(status: Status) -> str
```

The Python package includes type stubs (`__init__.pyi`) for full IDE autocomplete and type checking support with mypy, pyright, etc.

## Configuration

Status trees are defined in JSON configuration files. See [CONFIG_FORMAT.md](CONFIG_FORMAT.md) for detailed documentation on:

- Node types (imported vs derived)
- Rollup rules and their parameters
- Dependency specification
- Example configurations

## Dependencies

All dependencies are automatically downloaded and built:

### C++ Dependencies (via CMake FetchContent)
- [CGraph](https://github.com/ChunelFeng/CGraph) v3.1.2 - DAG computation framework
- [nlohmann/json](https://github.com/nlohmann/json) v3.12.0 - JSON parsing
- [GoogleTest](https://github.com/google/googletest) v1.14.0 - Testing framework (test builds only)

### Python Dependencies (via pyproject.toml)
- [pybind11](https://github.com/pybind/pybind11) v2.12.0 - Python bindings
- [scikit-build-core](https://github.com/scikit-build/scikit-build-core) - Build backend for Python packages with CMake
- Python 3.8+ required

## Testing

The project includes comprehensive tests:

```bash
# Run all tests
cd build && ctest

# Run with verbose output
cd build && ctest --output-on-failure

# Run specific test executable
./build/test_rollup_rules
./build/test_integration

# Run shell script tests manually
bash tests/test_interactive.sh build/rollup complex_status_config.json
```

Test coverage:
- **Unit tests** (16 tests): Rollup rule logic, status conversions, edge cases
- **Integration tests** (12 tests): StatusTree API, error handling, propagation
- **Interactive tests** (10 tests): End-to-end command-line interface testing
- **Python tests** (18 tests): Python bindings functionality, all rollup rules

## Implementation Details

- **C++20**: Uses modern C++ features 
- **Pimpl Pattern**: CGraph dependency hidden from public headers
- **Warning-Free**: Strict compiler warnings (-Wall -Wextra -Wpedantic -Werror) with third-party warnings suppressed
- **Thread Safety**: Uses CGraph's thread pool for parallel computation
- **Topological Ordering**: Automatic dependency resolution handles arbitrary configuration order

# Responsible LLM Use Disclosure
This repository contains content generated or assisted by Large Language Models
(LLMs). While LLMs can be powerful tools for augmenting human capabilities, they
are not a replacement for critical thinking and intellectual rigor. The author
of this document has made efforts to ensure that any LLM-generated content is
accurately attributed and transparently disclosed. However, readers should be
aware the LLMs may introduce subtle biases, errors, or limitations that are not
immediately apparent.

To maintain the integrity and accountability of the work, the author commits to
the following principles:
* Training: efforts have been made to stay current with the latest developments
  and limitations of LLMs, and to use the judiciously and with careful
  consideration
* Transparency: we acknowledge the use of LLMs in this codebase and disclose any
  generated content
* Traceability: we make available, upon request, the prompts and inputs used to
  generate any LLM-assisted content

Readers are encouraged to critically evaluate the content of this document,
considering the potential risks and limitations associated with LLM-generated
material. 

For more information, see https://colinmilhaupt.com/posts/responsible-llm-use/
