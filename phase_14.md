# Phase 14 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-14`

## Objective

Phase 14 continues the maintenance/services track with a read-only limbo-transaction listing helper.

The goal is to expose one more repair-oriented service action without introducing destructive recovery behavior.

## Version Target

Phase 14 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: limbo-list helper on `FirebirdDatabase`

Implement:

- `ListLimboTransactions() As Boolean`

This should run Firebird's service-manager repair action with the limbo-list option for the connected database.

### Deliverable 2: desktop integration coverage

Add desktop integration tests that prove:

- the limbo-list action succeeds
- the service output path remains stable
- clean databases are handled without forcing a synthetic limbo state

## In Scope

- Firebird service-manager repair action via `isc_action_svc_repair`
- `isc_spb_rpr_list_limbo_trans`
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage

## Out of Scope

- commit/rollback/recover limbo workflows
- synthetic two-phase transaction generation
- database shutdown / bring-online actions
- broader maintenance-services work
- documentation closeout before verification

## Design Rules

### Rule 1: keep Phase 14 read-only

Expose only the limbo-list helper, not recovery or mutation helpers.

### Rule 2: accept clean-database output behavior

The verification path must allow success both with diagnostic output and with an empty result on a clean database.

### Rule 3: keep service output handling conservative

Stay on the stable service-query path already in use by the plugin.

## Success Criteria

Phase 14 is complete when:

- `ListLimboTransactions()` works
- the desktop suite proves the clean-database path behaves correctly
- the suite passes with the new coverage included
- final verification is `141 passed, 0 failed`
- docs are updated
- the detailed phase article is written
