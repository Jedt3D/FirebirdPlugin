# Phase 08 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-08`

## Objective

Phase 08 extends the service-manager operational surface with non-destructive database validation.

The goal is to expose a safe validation workflow without opening the broader repair surface yet.

## Version Target

Phase 08 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: validation helper on `FirebirdDatabase`

Implement:

- `ValidateDatabase() As Boolean`

This should run a non-destructive validation request through Firebird's service manager for the currently connected database.

### Deliverable 2: validation output reuse through `LastServiceOutput`

Reuse the existing service-output surface:

- `LastServiceOutput() As String`

After `ValidateDatabase()`, it should contain any diagnostic output returned by the validation run.

### Deliverable 3: desktop integration coverage

Add a desktop integration test that proves:

- `ValidateDatabase()` succeeds on the test database
- the service-output path remains stable
- the test tolerates a clean database returning little or no validation text

## In Scope

- Firebird service-manager validation via `isc_action_svc_validate`
- non-destructive validation options only
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage for validation
- README and example updates

## Out of Scope

- destructive repair options
- mend, sweep, or limbo-transaction recovery
- user-management services
- trace services
- a generic service-manager abstraction

## Design Rules

### Rule 1: keep Phase 08 non-destructive

Use validation-only options. Do not expose repair or mend behavior in this phase.

### Rule 2: reuse the connected database identity

Validation should run against the currently connected database using the same:

- host
- port
- user
- password
- role
- database path

### Rule 3: reuse the existing service-output channel

Do not add another reporting API. Use `LastServiceOutput()` consistently for backup, restore, statistics, and validation.

## Success Criteria

Phase 08 is complete when:

- `ValidateDatabase()` works
- `LastServiceOutput()` remains stable for validation runs
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
- final verification result is `113 passed, 0 failed`
