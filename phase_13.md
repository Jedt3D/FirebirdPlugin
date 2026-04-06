# Phase 13 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-13`

## Objective

Phase 13 returns to the maintenance/services track with a narrow database-sweep helper.

The goal is to expose one safe maintenance action that can be verified without changing the plugin's broader connection or transaction model.

## Version Target

Phase 13 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: sweep helper on `FirebirdDatabase`

Implement:

- `SweepDatabase() As Boolean`

This should run Firebird's service-manager repair action with the sweep option for the connected database.

### Deliverable 2: desktop integration coverage

Add desktop integration tests that prove:

- the sweep action succeeds
- the service output path remains stable
- the database remains queryable after the sweep

## In Scope

- Firebird service-manager repair action via `isc_action_svc_repair`
- `isc_spb_rpr_sweep_db`
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage

## Out of Scope

- database shutdown / bring-online actions
- mend / repair workflows
- limbo transaction recovery
- broader maintenance-services work
- documentation closeout before verification

## Design Rules

### Rule 1: keep Phase 13 narrow

Expose only the sweep helper, not a larger repair API surface.

### Rule 2: prefer non-destructive verification

Verify the sweep by successful service execution and a normal SQL query afterward, not by forcing corruption or shutdown scenarios.

### Rule 3: keep service output handling conservative

Stay on the stable service-query path already in use by the plugin.

## Success Criteria

Phase 13 is complete when:

- `SweepDatabase()` works
- the desktop suite proves the database remains usable after a sweep
- the suite passes with the new coverage included
- final verification is `138 passed, 0 failed`
- docs are updated
- the detailed phase article is written
