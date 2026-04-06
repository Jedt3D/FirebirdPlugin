# Phase 09 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-09`

## Objective

Phase 09 extends the Services API with a narrow, read-only user-management slice.

The goal is to expose user display/listing through Firebird's service manager without opening user-creation, password-change, or delete-user workflows yet.

## Version Target

Phase 09 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: read-only user display helper on `FirebirdDatabase`

Implement:

- `DisplayUsers() As Boolean`

This should run Firebird's security/user display service action and populate `LastServiceOutput()` with the returned listing.

### Deliverable 2: service-output reuse through `LastServiceOutput`

Reuse the existing service-output surface:

- `LastServiceOutput() As String`

After `DisplayUsers()`, it should contain the returned user listing.

### Deliverable 3: desktop integration coverage

Add a desktop integration test that proves:

- `DisplayUsers()` succeeds with the current admin credentials
- the service-output path remains stable
- the output includes `SYSDBA`

## In Scope

- Firebird service-manager user display via `isc_action_svc_display_user`
- read-only user listing only
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage
- README and example updates

## Out of Scope

- add user
- modify user
- delete user
- password management
- role or admin-flag mutation
- a generic security/user object model

## Design Rules

### Rule 1: keep Phase 09 read-only

Do not expose mutating user-management behavior in this phase.

### Rule 2: reuse the existing service-output channel

Do not add another reporting API. Use `LastServiceOutput()` consistently for backup, restore, statistics, validation, and user display.

### Rule 3: keep the public Xojo surface narrow

Prefer one stable listing method over a broader but unverified CRUD surface.

## Success Criteria

Phase 09 is complete when:

- `DisplayUsers()` works
- `LastServiceOutput()` remains stable for user display runs
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
- final verification result is `116 passed, 0 failed`
