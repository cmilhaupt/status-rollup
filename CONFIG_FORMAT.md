# Configuration File Format Reference

This document describes the JSON configuration format for defining status rollup trees.

## Overview

A configuration file defines a directed acyclic graph (DAG) of status nodes. Each node is either:
- **Imported**: A leaf node with status set externally (e.g., from monitoring systems)
- **Derived**: An intermediate/root node with status computed from dependencies using a rollup rule

## File Structure

```json
{
  "nodes": {
    "node_name_1": { /* node configuration */ },
    "node_name_2": { /* node configuration */ },
    ...
  }
}
```

The top-level object contains a single `nodes` object where keys are node names and values are node configurations.

## Node Configuration

### Imported Nodes (Leaf Nodes)

Imported nodes represent externally-provided status values:

```json
{
  "type": "imported"
}
```

**Fields:**
- `type` (string, required): Must be `"imported"`

**Example:**
```json
{
  "nodes": {
    "db_primary": {
      "type": "imported"
    },
    "web_server_1": {
      "type": "imported"
    }
  }
}
```

### Derived Nodes (Computed Nodes)

Derived nodes compute their status from dependencies using a rollup rule:

```json
{
  "type": "derived",
  "rule": "rule_name",
  "params": { /* rule-specific parameters */ },
  "dependencies": ["node1", "node2", ...]
}
```

**Fields:**
- `type` (string, required): Must be `"derived"`
- `rule` (string, required): Name of the rollup rule (see below)
- `params` (object, optional): Rule-specific parameters
- `dependencies` (array, required): List of node names this node depends on

**Example:**
```json
{
  "nodes": {
    "database_cluster": {
      "type": "derived",
      "rule": "threshold_rollup",
      "params": {
        "red_threshold": 2,
        "yellow_to_yellow": 1,
        "yellow_to_red": 3
      },
      "dependencies": ["db_primary", "db_replica_1", "db_replica_2"]
    }
  }
}
```

## Rollup Rules

### 1. Worst Status Rule

Propagates the worst (most severe) status among all dependencies.

**Rule Name:** `"worst_status"`

**Parameters:** None

**Logic:**
- If any dependency is Red → Red
- Else if any dependency is Yellow → Yellow
- Else if all dependencies are Green → Green
- Else → Unknown

**Use Cases:**
- Critical components where any failure is critical
- Service health checks where all must pass
- Security monitoring where any alert is significant

**Example:**
```json
{
  "critical_services": {
    "type": "derived",
    "rule": "worst_status",
    "dependencies": ["auth_service", "payment_service", "user_db"]
  }
}
```

**Behavior Examples:**
- Dependencies: `[Green, Green, Green]` → `Green`
- Dependencies: `[Green, Yellow, Green]` → `Yellow`
- Dependencies: `[Green, Green, Red]` → `Red`
- Dependencies: `[Yellow, Yellow, Red]` → `Red`

---

### 2. Threshold Rollup Rule

Uses configurable thresholds to determine when accumulated issues trigger status changes.

**Rule Name:** `"threshold_rollup"`

**Parameters:**
- `red_threshold` (integer, required): Number of Red dependencies to trigger Red status
- `yellow_to_yellow` (integer, required): Number of Yellow dependencies to trigger Yellow status
- `yellow_to_red` (integer, required): Number of Yellow dependencies to trigger Red status

**Logic:**
1. Count Red dependencies. If count ≥ `red_threshold` → Red
2. Count Yellow dependencies:
   - If count ≥ `yellow_to_red` → Red
   - Else if count ≥ `yellow_to_yellow` → Yellow
3. Otherwise → Green (assuming all remaining are Green)

**Use Cases:**
- Redundant systems where single failures are acceptable
- Load-balanced services with multiple instances
- Replica sets where some degradation is tolerable

**Example:**
```json
{
  "api_cluster": {
    "type": "derived",
    "rule": "threshold_rollup",
    "params": {
      "red_threshold": 2,      // 2+ red instances → cluster is red
      "yellow_to_yellow": 1,   // 1+ yellow instance → cluster is yellow
      "yellow_to_red": 3       // 3+ yellow instances → cluster is red
    },
    "dependencies": ["api_server_1", "api_server_2", "api_server_3", "api_server_4"]
  }
}
```

**Behavior Examples:**

With `red_threshold=2, yellow_to_yellow=1, yellow_to_red=3`:

| Dependencies | Result | Reason |
|-------------|--------|--------|
| `[Green, Green, Green, Green]` | Green | All healthy |
| `[Green, Yellow, Green, Green]` | Yellow | 1 yellow ≥ yellow_to_yellow |
| `[Green, Yellow, Yellow, Green]` | Yellow | 2 yellows < yellow_to_red |
| `[Yellow, Yellow, Yellow, Green]` | Red | 3 yellows ≥ yellow_to_red |
| `[Green, Red, Green, Green]` | Green | 1 red < red_threshold |
| `[Red, Red, Green, Green]` | Red | 2 reds ≥ red_threshold |
| `[Red, Yellow, Yellow, Yellow]` | Red | 1 red < threshold but 3 yellows ≥ yellow_to_red |

---

### 3. Majority Vote Rule

Status determined by majority voting among dependencies. In case of ties, the more severe status wins.

**Rule Name:** `"majority_vote"`

**Parameters:** None

**Logic:**
1. Count votes for each status (Green, Yellow, Red)
2. Status with most votes wins
3. Tie-breaking: Red > Yellow > Green > Unknown

**Use Cases:**
- Consensus-based systems
- Distributed systems where majority agreement indicates health
- Voting panels or multi-region deployments

**Example:**
```json
{
  "multi_region_service": {
    "type": "derived",
    "rule": "majority_vote",
    "dependencies": ["us_east", "us_west", "eu_central", "asia_pacific"]
  }
}
```

**Behavior Examples:**

| Dependencies | Votes | Result | Reason |
|-------------|-------|--------|--------|
| `[Green, Green, Yellow]` | G:2, Y:1 | Green | Green majority |
| `[Yellow, Yellow, Green]` | Y:2, G:1 | Yellow | Yellow majority |
| `[Red, Red, Green]` | R:2, G:1 | Red | Red majority |
| `[Green, Yellow, Red]` | G:1, Y:1, R:1 | Red | 3-way tie, Red wins |
| `[Green, Green, Yellow, Yellow]` | G:2, Y:2 | Yellow | Green-Yellow tie, Yellow wins |
| `[Green, Yellow, Red, Unknown]` | G:1, Y:1, R:1, U:1 | Red | 4-way tie, Red wins |

---

## Complete Example

Here's a complete configuration for a microservices architecture:

```json
{
  "nodes": {
    "auth_service": {
      "type": "imported"
    },
    "user_service": {
      "type": "imported"
    },
    "payment_service": {
      "type": "imported"
    },

    "api_server_1": {
      "type": "imported"
    },
    "api_server_2": {
      "type": "imported"
    },
    "api_server_3": {
      "type": "imported"
    },

    "db_primary": {
      "type": "imported"
    },
    "db_replica_1": {
      "type": "imported"
    },
    "db_replica_2": {
      "type": "imported"
    },

    "microservices": {
      "type": "derived",
      "rule": "worst_status",
      "dependencies": ["auth_service", "user_service", "payment_service"]
    },

    "api_cluster": {
      "type": "derived",
      "rule": "threshold_rollup",
      "params": {
        "red_threshold": 2,
        "yellow_to_yellow": 1,
        "yellow_to_red": 3
      },
      "dependencies": ["api_server_1", "api_server_2", "api_server_3"]
    },

    "database": {
      "type": "derived",
      "rule": "majority_vote",
      "dependencies": ["db_primary", "db_replica_1", "db_replica_2"]
    },

    "overall_health": {
      "type": "derived",
      "rule": "worst_status",
      "dependencies": ["microservices", "api_cluster", "database"]
    }
  }
}
```

## Dependency Graph

The dependency graph must be acyclic. The system automatically handles topological ordering, so nodes can be defined in any order in the JSON file.

### Valid Graph

```
         overall_health (worst_status)
         /        |           \
        /         |            \
  microservices  api_cluster  database
  (worst_status) (threshold)  (majority_vote)
      /  |  \      /  |  \       /    |    \
     /   |   \    /   |   \     /     |     \
  auth user pay api1 api2 api3 db_p db_r1 db_r2
  (leaf) (leaf)... (all leaf nodes)
```

### Invalid Graph (Cycle)

```
  node_a → node_b → node_c
    ↑                  ↓
    └──────────────────┘
```

**Error:** Cyclic dependencies are not supported and will cause undefined behavior.

## Validation Rules

The configuration file must satisfy these constraints:

1. **Unique node names**: Each node name must be unique
2. **Valid node types**: Must be either `"imported"` or `"derived"`
3. **Required fields**:
   - Imported nodes: `type`
   - Derived nodes: `type`, `rule`, `dependencies`
4. **Valid rule names**: Must be `"worst_status"`, `"threshold_rollup"`, or `"majority_vote"`
5. **Valid dependencies**: All dependency names must refer to existing nodes
6. **No cycles**: Dependency graph must be acyclic
7. **Required parameters**: Rule-specific parameters must be provided where required
8. **Valid threshold values**: Threshold parameters must be non-negative integers

## Best Practices

### Naming Conventions

- Use descriptive, hierarchical names: `platform_delivery_api_cluster`
- Use underscores for multi-word names: `user_service`, not `userService` or `user-service`
- Group related nodes with common prefixes: `db_primary`, `db_replica_1`, `db_replica_2`

### Tree Design

- **Keep depth reasonable**: 3-5 levels is typical; deeper trees are harder to understand
- **Balance fanout**: 3-7 dependencies per node is ideal; too few or too many reduces clarity
- **Choose appropriate rules**: Match rule semantics to component relationships
- **Use worst_status conservatively**: Reserve for truly critical components
- **Leverage threshold_rollup**: Best for redundant systems with graceful degradation

### Parameter Selection

For `threshold_rollup`:
- Set `red_threshold` based on minimum required healthy instances
- Set `yellow_to_yellow = 1` for early warning on any degradation
- Set `yellow_to_red` to trigger red before complete failure (e.g., if you need 3 of 5 instances, set it to 3)

### Example: Load Balanced Service

```json
{
  "web_cluster": {
    "type": "derived",
    "rule": "threshold_rollup",
    "params": {
      "red_threshold": 3,      // Need at least 3 healthy servers
      "yellow_to_yellow": 1,   // Warn on any degradation
      "yellow_to_red": 4       // Critical if 4+ are degraded
    },
    "dependencies": ["web1", "web2", "web3", "web4", "web5", "web6"]
  }
}
```

This configuration allows:
- Up to 2 failed servers while staying green
- Warning status if any server is degraded
- Critical status if 3+ servers fail OR 4+ servers are degraded

## Common Patterns

### Pattern 1: Redundant Services

Use `threshold_rollup` for services with multiple instances:

```json
{
  "cache_cluster": {
    "type": "derived",
    "rule": "threshold_rollup",
    "params": {
      "red_threshold": 2,
      "yellow_to_yellow": 1,
      "yellow_to_red": 2
    },
    "dependencies": ["cache_1", "cache_2", "cache_3"]
  }
}
```

### Pattern 2: Critical Dependency Chain

Use `worst_status` when all components must be healthy:

```json
{
  "payment_pipeline": {
    "type": "derived",
    "rule": "worst_status",
    "dependencies": ["payment_api", "payment_processor", "payment_db"]
  }
}
```

### Pattern 3: Multi-Region Consensus

Use `majority_vote` for distributed regions:

```json
{
  "global_service": {
    "type": "derived",
    "rule": "majority_vote",
    "dependencies": ["region_us", "region_eu", "region_asia"]
  }
}
```

### Pattern 4: Layered Architecture

Combine rules at different layers:

```json
{
  "frontend_tier": {
    "type": "derived",
    "rule": "threshold_rollup",
    "params": {"red_threshold": 2, "yellow_to_yellow": 1, "yellow_to_red": 3},
    "dependencies": ["web1", "web2", "web3", "web4"]
  },
  "backend_tier": {
    "type": "derived",
    "rule": "worst_status",
    "dependencies": ["api_server", "message_queue", "cache"]
  },
  "data_tier": {
    "type": "derived",
    "rule": "majority_vote",
    "dependencies": ["db_primary", "db_replica_1", "db_replica_2"]
  },
  "overall_system": {
    "type": "derived",
    "rule": "worst_status",
    "dependencies": ["frontend_tier", "backend_tier", "data_tier"]
  }
}
```

## Troubleshooting

### Issue: Unexpected Status Values

**Symptom:** Node shows unexpected status (often `unknown`)

**Causes:**
1. Dependencies not properly initialized
2. Cyclic dependencies (undefined behavior)
3. Missing dependency node

**Solution:** Verify all leaf nodes are set and dependency graph is acyclic

### Issue: Status Not Propagating

**Symptom:** Updating leaf node doesn't affect derived nodes

**Cause:** Forgot to call `compute()` after `set_status()`

**Solution:** Always call `tree.compute()` after updating statuses

### Issue: Threshold Rule Not Working as Expected

**Symptom:** Wrong status from threshold rule

**Cause:** Misunderstanding of parameter meanings

**Solution:** Review the logic - yellow counts are checked AFTER red counts

## See Also

- [README.md](README.md) - Project overview and getting started guide
- [complex_status_config.json](complex_status_config.json) - 40-node example configuration
