#!/bin/bash

# Integration tests for the interactive application
# These test the full end-to-end behavior by piping commands via stdin

ROLLUP_BIN="$1"
CONFIG_FILE="$2"

if [ ! -x "$ROLLUP_BIN" ]; then
    echo "Error: Rollup binary not found or not executable: $ROLLUP_BIN"
    exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Config file not found: $CONFIG_FILE"
    exit 1
fi

TESTS_PASSED=0
TESTS_FAILED=0

# Helper function to run a test
run_test() {
    local test_name="$1"
    local commands="$2"
    local expected_pattern="$3"

    echo "Running: $test_name"

    output=$(echo -e "$commands" | "$ROLLUP_BIN" "$CONFIG_FILE" 2>&1)

    if echo "$output" | grep -q "$expected_pattern"; then
        echo "  ✓ PASSED"
        ((TESTS_PASSED++))
    else
        echo "  ✗ FAILED"
        echo "  Expected pattern: $expected_pattern"
        echo "  Output:"
        echo "$output" | head -20
        ((TESTS_FAILED++))
    fi
    return 0
}

# Test 1: Initial state should be all green
run_test "Initial state all green" \
    "get overall_system_health\nquit" \
    "overall_system_health: green"

# Test 2: Set a service to red
run_test "Set payment_service to red" \
    "payment_service red\nget payment_service\nquit" \
    "payment_service: red"

# Test 3: Red propagates to overall health
run_test "Red propagates to overall health" \
    "payment_service red\nget overall_system_health\nquit" \
    "overall_system_health: red"

# Test 4: Get intermediate node status
run_test "Get microservices status after payment_service red" \
    "payment_service red\nget microservices\nquit" \
    "microservices: red"

# Test 5: Yellow status
run_test "Set service to yellow" \
    "db_replica_2 yellow\nget db_replica_2\nquit" \
    "db_replica_2: yellow"

# Test 6: Multiple updates (2 yellows triggers red due to threshold rule)
run_test "Multiple status updates" \
    "api_server_1 yellow\napi_server_2 yellow\nget api_cluster\nquit" \
    "api_cluster: red"

# Test 7: Print command works
run_test "Print command shows tree" \
    "print\nquit" \
    "DERIVED NODES"

# Test 8: Invalid node name
run_test "Invalid node name error" \
    "nonexistent red\nquit" \
    "does not exist"

# Test 9: Invalid status value
run_test "Invalid status value error" \
    "db_primary purple\nquit" \
    "Invalid status"

# Test 10: Get non-existent node
run_test "Get non-existent node" \
    "get nonexistent\nquit" \
    "does not exist"

echo ""
echo "================================"
echo "Test Results:"
echo "  Passed: $TESTS_PASSED"
echo "  Failed: $TESTS_FAILED"
echo "================================"

if [ $TESTS_FAILED -gt 0 ]; then
    exit 1
fi

exit 0
