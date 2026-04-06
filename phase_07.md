# Phase 07 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `110 passed, 0 failed`
- article: `phase_07_article.md`

Branch:

- `feature/phase-07`

## Objective

Phase 07 extends the new Services API foundation from Phase 06 with a narrow database-statistics slice.

The goal is to expose Firebird's service-manager statistics workflow in a simple Xojo-friendly form without expanding into a broad administrative API yet.

## Version Target

Phase 07 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: statistics helper on `FirebirdDatabase`

Implement:

- `DatabaseStatistics() As Boolean`

This should run Firebird database statistics through the service manager for the currently connected database.

### Deliverable 2: statistics output reuse through `LastServiceOutput`

Reuse the existing service-output surface from Phase 06:

- `LastServiceOutput() As String`

After `DatabaseStatistics()`, it should contain the `gstat` textual report.

### Deliverable 3: desktop integration coverage

Add a desktop integration test that proves:

- `DatabaseStatistics()` succeeds
- `LastServiceOutput()` is non-empty
- the statistics output contains expected `gstat` report text

## In Scope

- Firebird service-manager database statistics via `isc_action_svc_db_stats`
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage for statistics
- README and example updates

## Out of Scope

- custom per-table statistics
- user-management services
- validation and repair services
- trace services
- a generic service-manager object model

## Design Rules

### Rule 1: keep the method narrow

Phase 07 should expose one simple statistics entry point instead of a large matrix of `gstat` flags.

### Rule 2: reuse the connected database identity

Statistics should run against the currently connected database using the same:

- host
- port
- user
- password
- role
- database path

### Rule 3: reuse the existing service-output channel

Do not add a second output surface. Use `LastServiceOutput()` consistently for backup, restore, and statistics.

## Success Criteria

Phase 07 is complete when:

- `DatabaseStatistics()` works
- `LastServiceOutput()` contains the statistics report
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
