# Phase 12 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-12`

## Objective

Phase 12 extends the Services API user-management track with a narrow modify-user slice for existing accounts.

The goal is to expose two focused operations without introducing a broad user object model:

- admin-flag mutation
- first/middle/last name updates

## Version Target

Phase 12 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: admin-flag helper on `FirebirdDatabase`

Implement:

- `SetUserAdmin(userName As String, isAdmin As Boolean) As Boolean`

This should run Firebird's modify-user service action for the target account with the admin flag set or cleared.

### Deliverable 2: name-update helper on `FirebirdDatabase`

Implement:

- `UpdateUserNames(userName As String, firstName As String, middleName As String, lastName As String) As Boolean`

This should run Firebird's modify-user service action for the target account with updated name fields.

### Deliverable 3: desktop integration coverage

Add desktop integration tests that prove:

- admin flag can be enabled and disabled
- enabled admin flag produces observable admin behavior
- admin flag readback is visible through `SEC$USERS`
- updated name fields are visible through `SEC$USERS`
- temporary users are cleaned up

## In Scope

- Firebird service-manager modify-user via `isc_action_svc_modify_user`
- `isc_spb_sec_admin`
- `isc_spb_sec_firstname`
- `isc_spb_sec_middlename`
- `isc_spb_sec_lastname`
- typed Xojo methods on `FirebirdDatabase`
- desktop integration coverage
- README and example updates

## Out of Scope

- generic user object APIs
- role management
- UID/GID mutation
- broader maintenance-services work

## Design Rules

### Rule 1: keep Phase 12 narrow

Expose only the two targeted modify-user helpers, not a general mutation object or builder.

### Rule 2: verify admin behavior, not just service output

Use a real login with the modified user and a service action to confirm the admin flag has observable effect.

### Rule 3: verify authoritative user fields directly

Use authoritative `SEC$USERS` readback for admin-flag and full-name verification.

### Rule 4: keep tests self-cleaning

All temporary users created by the desktop suite must be removed before the tests finish.

## Success Criteria

Phase 12 is complete when:

- `SetUserAdmin()` works
- `UpdateUserNames()` works
- desktop tests prove admin and name-update behavior
- the suite passes with the new coverage included
- final verification is `135 passed, 0 failed`
- docs are updated
- the detailed phase article is written
