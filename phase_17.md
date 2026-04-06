# Phase 17 Detailed Plan

## Status

Complete on April 6, 2026.

Branch:

- `feature/phase-17`

## Objective

Phase 17 continues the maintenance/services track with controlled shutdown and online helpers.

The goal is to expose a safe availability-control slice that can be verified locally through a dedicated service-control object instead of trying to drive shutdown from the same live SQL session.

## Version Target

Phase 17 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: shutdown / online helpers on `FirebirdDatabase`

Implement:

- `ShutdownDenyNewAttachments(timeoutSeconds As Integer) As Boolean`
- `BringDatabaseOnline() As Boolean`

These run Firebird's service-manager properties action using denied new attachments in multi-user shutdown mode, and a normal online transition.

### Deliverable 2: desktop integration coverage

Add desktop integration tests that prove:

- the shutdown helper is exposed to Xojo
- new ordinary-user attachments are denied while the database is in shutdown mode
- the online helper restores ordinary-user attachment success
- the shutdown and online requests are issued through a service-only control context
- the test always attempts to return the database online before cleanup

## In Scope

- Firebird service-manager properties action via `isc_action_svc_properties`
- `isc_spb_prp_shutdown_mode`
- `isc_spb_prp_attachments_shutdown`
- `isc_spb_prp_online_mode`
- service-only shutdown / online control context
- typed Xojo methods on `FirebirdDatabase`
- desktop integration coverage

## Out of Scope

- force shutdown
- deny-new-transactions shutdown mode
- single-user or full shutdown modes
- destructive server-wide shutdown behavior
- documentation closeout before verification

## Design Rules

### Rule 1: keep Phase 17 attachment-safe

Use multi-user shutdown mode and a separate service-control object so the controlling `SYSDBA` service session can bring the database back online cleanly.

### Rule 2: verify denial with an ordinary user

Do not use `SYSDBA` for the denied-attachment probe, because multi-user shutdown mode can still allow owner-level connections.

### Rule 3: always bias cleanup toward bringing the database online

If shutdown succeeds, the test must attempt `BringDatabaseOnline()` during cleanup even if a later assertion fails.

## Success Criteria

Phase 17 is complete when:

- `ShutdownDenyNewAttachments()` works
- `BringDatabaseOnline()` works
- the desktop suite proves ordinary-user attachment denial and restoration
- the service-control path avoids the earlier secondary-attachment failure mode
- the suite passes with the new coverage included
- docs are updated
- the detailed phase article is written

## Verification

Verified with the desktop suite on April 6, 2026:

- `157 passed`
- `0 failed`
