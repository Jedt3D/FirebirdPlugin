# Phase 10 Detailed Plan

## Status

Completed on April 6, 2026.

Branch:

- `feature/phase-10`

## Objective

Phase 10 extends the Services API with a narrow mutating user-management slice.

The goal is to expose basic add-user and delete-user workflows through Firebird's service manager without opening broader account-profile editing or password-management behavior yet.

## Version Target

Phase 10 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: basic user mutation helpers on `FirebirdDatabase`

Implement:

- `AddUser(userName As String, password As String) As Boolean`
- `DeleteUser(userName As String) As Boolean`

These should run Firebird's security/user service actions for add and delete.

### Deliverable 2: reuse `DisplayUsers()` and `LastServiceOutput()`

Reuse the existing service-user-display path:

- `DisplayUsers() As Boolean`
- `LastServiceOutput() As String`

This phase should use display output to verify the mutation behavior rather than creating a second reporting surface.

### Deliverable 3: desktop integration coverage

Add a desktop integration test that proves:

- a test user can be added
- the user appears in displayed users
- the same user can be deleted
- the user disappears from displayed users

## In Scope

- Firebird service-manager add-user via `isc_action_svc_add_user`
- Firebird service-manager delete-user via `isc_action_svc_delete_user`
- typed Xojo methods on `FirebirdDatabase`
- desktop integration coverage
- README and example updates

## Out of Scope

- modify user profile fields
- password changes for existing users
- admin-flag mutation
- role management
- a generic security/user object model

## Design Rules

### Rule 1: keep Phase 10 minimal

Expose only add and delete. Do not broaden the surface to full user CRUD in this phase.

### Rule 2: keep verification read-back simple

Verify mutation behavior through `DisplayUsers()` and `LastServiceOutput()`.

### Rule 3: keep the test self-cleaning

The desktop test must remove its temporary user so the suite remains rerunnable.

## Success Criteria

Phase 10 is complete when:

- `AddUser()` works
- `DeleteUser()` works
- display-based readback proves both mutation paths
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
- final verification result is `120 passed, 0 failed`
