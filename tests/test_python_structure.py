#!/usr/bin/env python3
"""Test that the Python package structure is correct."""

import sys
from pathlib import Path


def test_python_package_structure():
    """Verify Python package files exist and are valid."""
    root = Path(__file__).parent.parent

    # Check pyproject.toml exists and has required sections
    pyproject = root / "pyproject.toml"
    assert pyproject.exists(), "pyproject.toml not found"

    content = pyproject.read_text()
    assert "[build-system]" in content, "Missing [build-system] section"
    assert "[project]" in content, "Missing [project] section"
    assert "pybind11" in content, "Missing pybind11 dependency"
    assert "scikit-build-core" in content, "Missing scikit-build-core dependency"

    # Check Python bindings source
    bindings = root / "python" / "status_rollup_py.cpp"
    assert bindings.exists(), "Python bindings source not found"

    bindings_content = bindings.read_text()
    assert "PYBIND11_MODULE" in bindings_content, "Missing PYBIND11_MODULE"
    assert "_status_rollup" in bindings_content, "Module not named correctly"

    # Check Python package directory
    pkg_dir = root / "status_rollup"
    assert pkg_dir.is_dir(), "status_rollup package directory not found"

    # Check __init__.py
    init_file = pkg_dir / "__init__.py"
    assert init_file.exists(), "__init__.py not found"

    init_content = init_file.read_text()
    assert "from ._status_rollup import" in init_content, "Missing import from C++ module"
    assert "Status" in init_content, "Missing Status export"
    assert "StatusTree" in init_content, "Missing StatusTree export"

    # Check type stubs
    pyi_file = pkg_dir / "__init__.pyi"
    assert pyi_file.exists(), "__init__.pyi not found"

    pyi_content = pyi_file.read_text()
    assert "class Status" in pyi_content, "Missing Status class in stubs"
    assert "class StatusTree" in pyi_content, "Missing StatusTree class in stubs"
    assert "def load_config" in pyi_content, "Missing load_config in stubs"

    # Check py.typed marker
    py_typed = pkg_dir / "py.typed"
    assert py_typed.exists(), "py.typed marker not found"

    # Check Python example
    example = root / "examples" / "python_example.py"
    assert example.exists(), "Python example not found"

    example_content = example.read_text()
    assert "from status_rollup import" in example_content, "Missing import in example"
    assert "def main()" in example_content, "Missing main function in example"

    # Check CMakeLists.txt has Python bindings option
    cmake_file = root / "CMakeLists.txt"
    assert cmake_file.exists(), "CMakeLists.txt not found"

    cmake_content = cmake_file.read_text()
    assert "BUILD_PYTHON_BINDINGS" in cmake_content, "Missing BUILD_PYTHON_BINDINGS option"
    assert "pybind11" in cmake_content, "Missing pybind11 in CMake"
    assert "pybind11_add_module" in cmake_content, "Missing pybind11_add_module"

    print("✓ All Python package structure checks passed")
    return True


if __name__ == "__main__":
    try:
        test_python_package_structure()
        sys.exit(0)
    except AssertionError as e:
        print(f"✗ Test failed: {e}")
        sys.exit(1)
    except (OSError, ValueError, RuntimeError) as e:
        print(f"✗ Unexpected error: {e}")
        sys.exit(1)
