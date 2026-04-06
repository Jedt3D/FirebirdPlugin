# Phase 11 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-11`

## Objective

Phase 11 extends the Services API with the next narrow user-management slice: password change for an existing user.

The goal is to expose password mutation through Firebird's service manager without opening broader profile-editing behavior yet.

## Version Target

Phase 11 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: password-change helper on `FirebirdDatabase`

Implement:

- `ChangeUserPassword(userName As String, password As String) As Boolean`

This should run Firebird's modify-user service action for the target account.

### Deliverable 2: authentication-based verification

Reuse the existing database connection surface to verify the behavior:

- original password works before change
- original password fails after change
- new password works after change

### Deliverable 3: desktop integration coverage

Add a desktop integration test that proves:

- a test user can be created
- the password can be changed
- old-password authentication stops working
- new-password authentication succeeds
- the test user is cleaned up

## In Scope

- Firebird service-manager modify-user via `isc_action_svc_modify_user`
- password mutation only
- typed Xojo method on `FirebirdDatabase`
- desktop integration coverage
- README and example updates

## Out of Scope

- first/middle/last name mutation
- admin-flag mutation
- role management
- a generic security/user object model

## Design Rules

### Rule 1: keep Phase 11 narrow

Expose only password change, not broader profile modification.

### Rule 2: verify behavior through actual login

Use real connect attempts to verify the old/new password behavior, not only service output.

### Rule 3: keep the test self-cleaning

The desktop test must remove its temporary user so the suite remains rerunnable.

## Success Criteria

Phase 11 is complete when:

- `ChangeUserPassword()` works
- authentication proves the old password is rejected and the new password is accepted
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written

## Result

Phase 11 completed with:

- `ChangeUserPassword(userName As String, password As String) As Boolean`
- authentication-based verification through the desktop integration suite
- documentation and example updates
- detailed phase article completion

Verification result:

- `125 passed, 0 failed`
