# Phase 16 Detailed Plan

## Status

Complete on April 6, 2026.

Branch:

- `feature/phase-16`

## Objective

Phase 16 continues the maintenance/repair track with explicit limbo-recovery helpers.

The goal is to expose narrow recovery methods without pulling shutdown or broader repair workflows into the same phase.

## Version Target

Phase 16 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: limbo-recovery helpers on `FirebirdDatabase`

Implement:

- `CommitLimboTransaction(transactionId As Int64) As Boolean`
- `RollbackLimboTransaction(transactionId As Int64) As Boolean`

These should run Firebird's service-manager repair action against a specific limbo transaction id.

### Deliverable 2: desktop integration coverage

Add desktop integration tests that prove:

- the recovery methods are exposed to Xojo
- a clean database can exercise the recovery path safely with a non-limbo transaction id
- the engine response on a non-limbo transaction remains safe whether it is an explicit rejection or a no-op acceptance
- the suite avoids mutating real limbo transactions if any are unexpectedly present

## In Scope

- Firebird service-manager repair action via `isc_action_svc_repair`
- `isc_spb_rpr_commit_trans_64`
- `isc_spb_rpr_rollback_trans_64`
- typed Xojo methods on `FirebirdDatabase`
- desktop integration coverage

## Out of Scope

- synthesizing a real distributed limbo transaction in the desktop suite
- shutdown / bring-online actions
- automatic limbo parsing / bulk recovery helpers
- documentation closeout before verification

## Design Rules

### Rule 1: keep Phase 16 transaction-targeted

Only expose commit/rollback by explicit transaction id in this phase.

### Rule 2: keep the local test non-destructive

If the database unexpectedly reports real limbo transactions, the test should skip destructive recovery attempts instead of auto-committing or auto-rolling them back.

### Rule 3: verify safe behavior on a clean database

Use a known non-limbo transaction id from a local explicit transaction and confirm the engine handles recovery without destabilizing the database, whether as rejection or no-op acceptance.

## Success Criteria

Phase 16 is complete when:

- `CommitLimboTransaction()` works
- `RollbackLimboTransaction()` works
- the desktop suite proves safe behavior on a clean database
- the suite passes with the new coverage included
- docs are updated
- the detailed phase article is written

## Verification

Verified with the desktop suite on April 6, 2026:

- `149 passed`
- `0 failed`
