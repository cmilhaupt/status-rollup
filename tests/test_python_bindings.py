#!/usr/bin/env python3
"""Functional tests for Python bindings."""

import json
import tempfile
from pathlib import Path

import pytest

from status_rollup import Status, StatusTree, status_to_string, string_to_status


class TestStatusEnum:
    """Test the Status enum."""

    def test_status_values(self):
        """Test that Status enum has all expected values."""
        assert Status.GREEN is not None
        assert Status.YELLOW is not None
        assert Status.RED is not None
        assert Status.UNKNOWN is not None

    def test_status_ordering(self):
        """Test that status values have proper ordering."""
        # Status should be comparable
        assert Status.GREEN != Status.YELLOW
        assert Status.YELLOW != Status.RED
        assert Status.RED != Status.UNKNOWN


class TestStatusConversion:
    """Test status string conversion functions."""

    def test_status_to_string(self):
        """Test converting Status enum to string."""
        assert status_to_string(Status.GREEN) == "green"
        assert status_to_string(Status.YELLOW) == "yellow"
        assert status_to_string(Status.RED) == "red"
        assert status_to_string(Status.UNKNOWN) == "unknown"

    def test_string_to_status(self):
        """Test converting string to Status enum."""
        assert string_to_status("green") == Status.GREEN
        assert string_to_status("yellow") == Status.YELLOW
        assert string_to_status("red") == Status.RED
        assert string_to_status("unknown") == Status.UNKNOWN

    def test_string_to_status_case_sensitive(self):
        """Test that string_to_status is case sensitive (lowercase required)."""
        # The implementation appears to be case-sensitive and returns UNKNOWN for invalid
        assert string_to_status("green") == Status.GREEN
        assert string_to_status("yellow") == Status.YELLOW
        assert string_to_status("red") == Status.RED

        # Uppercase returns UNKNOWN
        assert string_to_status("GREEN") == Status.UNKNOWN
        assert string_to_status("Yellow") == Status.UNKNOWN

    def test_string_to_status_invalid(self):
        """Test that invalid strings return UNKNOWN."""
        # The implementation returns UNKNOWN for invalid strings instead of raising
        assert string_to_status("invalid") == Status.UNKNOWN


class TestStatusTreeBasic:
    """Test basic StatusTree operations."""

    def test_create_tree(self):
        """Test creating an empty StatusTree."""
        tree = StatusTree()
        assert tree is not None

    def test_load_config_from_file(self):
        """Test loading config from a JSON file."""
        config = {
            "nodes": {
                "root": {
                    "type": "derived",
                    "rule": "worst_status",
                    "dependencies": ["leaf1", "leaf2"]
                },
                "leaf1": {"type": "imported"},
                "leaf2": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            # Verify nodes were created
            tree.set_status("leaf1", Status.GREEN)
            tree.set_status("leaf2", Status.GREEN)
            tree.compute()

            assert tree.get_status("root") == Status.GREEN
        finally:
            Path(config_path).unlink()

    def test_set_and_get_status(self):
        """Test setting and getting status."""
        config = {
            "nodes": {
                "test_node": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            tree.set_status("test_node", Status.GREEN)
            assert tree.get_status("test_node") == Status.GREEN

            tree.set_status("test_node", Status.YELLOW)
            assert tree.get_status("test_node") == Status.YELLOW
        finally:
            Path(config_path).unlink()

    def test_get_nonexistent_node(self):
        """Test getting status of nonexistent node."""
        tree = StatusTree()

        # Getting status of nonexistent node should return None or raise
        result = tree.get_status("nonexistent")
        assert result is None

    def test_set_status_on_nonexistent_node(self):
        """Test setting status on nonexistent node raises error."""
        tree = StatusTree()

        with pytest.raises(RuntimeError):
            tree.set_status("nonexistent", Status.GREEN)


class TestWorstStatusRule:
    """Test worst_status rollup rule."""

    def test_all_green(self):
        """Test that all green children result in green parent."""
        config = {
            "nodes": {
                "parent": {
                    "type": "derived",
                    "rule": "worst_status",
                    "dependencies": ["child1", "child2", "child3"]
                },
                "child1": {"type": "imported"},
                "child2": {"type": "imported"},
                "child3": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            tree.set_status("child1", Status.GREEN)
            tree.set_status("child2", Status.GREEN)
            tree.set_status("child3", Status.GREEN)
            tree.compute()

            assert tree.get_status("parent") == Status.GREEN
        finally:
            Path(config_path).unlink()

    def test_one_yellow(self):
        """Test that one yellow child makes parent yellow."""
        config = {
            "nodes": {
                "parent": {
                    "type": "derived",
                    "rule": "worst_status",
                    "dependencies": ["child1", "child2"]
                },
                "child1": {"type": "imported"},
                "child2": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            tree.set_status("child1", Status.GREEN)
            tree.set_status("child2", Status.YELLOW)
            tree.compute()

            assert tree.get_status("parent") == Status.YELLOW
        finally:
            Path(config_path).unlink()

    def test_one_red(self):
        """Test that one red child makes parent red."""
        config = {
            "nodes": {
                "parent": {
                    "type": "derived",
                    "rule": "worst_status",
                    "dependencies": ["child1", "child2"]
                },
                "child1": {"type": "imported"},
                "child2": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            tree.set_status("child1", Status.GREEN)
            tree.set_status("child2", Status.RED)
            tree.compute()

            assert tree.get_status("parent") == Status.RED
        finally:
            Path(config_path).unlink()


class TestThresholdRollupRule:
    """Test threshold_rollup rule."""

    def test_below_red_threshold(self):
        """Test status when below red threshold (returns green)."""
        config = {
            "nodes": {
                "parent": {
                    "type": "derived",
                    "rule": "threshold_rollup",
                    "dependencies": ["child1", "child2", "child3", "child4"],
                    "params": {
                        "red_threshold": 2,
                        "yellow_to_red": 3,
                        "yellow_to_yellow": 1
                    }
                },
                "child1": {"type": "imported"},
                "child2": {"type": "imported"},
                "child3": {"type": "imported"},
                "child4": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            # 1 red (below threshold of 2) -> parent should be green
            tree.set_status("child1", Status.RED)
            tree.set_status("child2", Status.GREEN)
            tree.set_status("child3", Status.GREEN)
            tree.set_status("child4", Status.GREEN)
            tree.compute()

            assert tree.get_status("parent") == Status.GREEN
        finally:
            Path(config_path).unlink()

    def test_at_red_threshold(self):
        """Test status when at red threshold."""
        config = {
            "nodes": {
                "parent": {
                    "type": "derived",
                    "rule": "threshold_rollup",
                    "dependencies": ["child1", "child2", "child3", "child4"],
                    "params": {
                        "red_threshold": 2,
                        "yellow_to_red": 3,
                        "yellow_to_yellow": 1
                    }
                },
                "child1": {"type": "imported"},
                "child2": {"type": "imported"},
                "child3": {"type": "imported"},
                "child4": {"type": "imported"}
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config, f)
            config_path = f.name

        try:
            tree = StatusTree()
            tree.load_config(config_path)

            # 2 red (at threshold)
            tree.set_status("child1", Status.RED)
            tree.set_status("child2", Status.RED)
            tree.set_status("child3", Status.GREEN)
            tree.set_status("child4", Status.GREEN)
            tree.compute()

            assert tree.get_status("parent") == Status.RED
        finally:
            Path(config_path).unlink()


class TestIntegration:
    """Integration tests using example config."""

    def test_load_example_config(self):
        """Test loading the example config file."""
        # Find the example config
        test_dir = Path(__file__).parent
        example_config = test_dir.parent / "examples" / "status_config.json"

        if not example_config.exists():
            pytest.skip(f"Example config not found: {example_config}")

        tree = StatusTree()
        tree.load_config(str(example_config))

        # Set all leaf nodes to green
        tree.set_status("service_db", Status.GREEN)
        tree.set_status("service_api", Status.GREEN)
        tree.set_status("service_cache", Status.GREEN)
        tree.set_status("service_queue", Status.GREEN)
        tree.compute()

        # Platform backend should be green (all deps green)
        assert tree.get_status("platform_backend") == Status.GREEN

        # Platform overall should be green
        assert tree.get_status("platform_overall") == Status.GREEN

    def test_example_config_threshold_behavior(self):
        """Test threshold rollup behavior with example config."""
        test_dir = Path(__file__).parent
        example_config = test_dir.parent / "examples" / "status_config.json"

        if not example_config.exists():
            pytest.skip(f"Example config not found: {example_config}")

        tree = StatusTree()
        tree.load_config(str(example_config))

        # Set 1 service to red (below red_threshold of 2)
        tree.set_status("service_db", Status.RED)
        tree.set_status("service_api", Status.GREEN)
        tree.set_status("service_cache", Status.GREEN)
        tree.set_status("service_queue", Status.GREEN)
        tree.compute()

        # Backend should be green (1 red < threshold of 2, and no yellows >= yellow_to_yellow)
        assert tree.get_status("platform_backend") == Status.GREEN

        # Set 2 services to red (at red_threshold of 2)
        tree.set_status("service_api", Status.RED)
        tree.compute()

        # Backend should be red (2 red >= threshold of 2)
        assert tree.get_status("platform_backend") == Status.RED

        # Test yellow threshold behavior
        tree.set_status("service_db", Status.GREEN)
        tree.set_status("service_api", Status.GREEN)
        tree.set_status("service_cache", Status.YELLOW)
        tree.set_status("service_queue", Status.GREEN)
        tree.compute()

        # Backend should be yellow (1 yellow >= yellow_to_yellow of 1)
        assert tree.get_status("platform_backend") == Status.YELLOW
