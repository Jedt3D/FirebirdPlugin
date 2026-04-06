# Phase 15 Detailed Plan

## Status

Complete on April 6, 2026.

Branch:

- `feature/phase-15`

## Objective

Phase 15 continues the maintenance/properties track with a reversible sweep-interval helper.

The goal is to expose one database-properties action that can be verified through a direct metadata readback without forcing shutdown or repair scenarios.

## Version Target

Phase 15 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: sweep-interval helper on `FirebirdDatabase`

Implement:

- `SetSweepInterval(interval As Integer) As Boolean`

This should run Firebird's service-manager properties action and update the database sweep interval.

### Deliverable 2: desktop integration coverage

Add desktop integration tests that prove:

- the property update succeeds
- the service output path remains stable
- the new value is visible through `MON$DATABASE`
- the original value is restored after the test

## In Scope

- Firebird service-manager properties action via `isc_action_svc_properties`
- `isc_spb_prp_sweep_interval`
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage

## Out of Scope

- write mode / forced writes properties
- read-only / access-mode property changes
- shutdown / bring-online actions
- broader maintenance-services work
- documentation closeout before verification

## Design Rules

### Rule 1: keep Phase 15 reversible

The test must restore the original sweep interval before it exits.

### Rule 2: prefer direct metadata readback

Verify the change through `MON$DATABASE.MON$SWEEP_INTERVAL`, not formatted service output.

### Rule 3: keep service output handling conservative

Allow the property change to succeed with either verbose output or no output.

## Success Criteria

Phase 15 is complete when:

- `SetSweepInterval()` works
- the desktop suite proves the value changes and is restored
- the suite passes with the new coverage included
- docs are updated
- the detailed phase article is written

## Verification

Verified with the desktop suite on April 6, 2026:

- `144 passed`
- `0 failed`
