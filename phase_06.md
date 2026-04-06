# Phase 06 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `107 passed, 0 failed`
- article: `phase_06_article.md`

Branch:

- `feature/phase-06`

## Objective

Phase 06 adds the first narrow Services API slice for the Xojo plugin:

- server-side backup
- server-side restore
- service-manager output capture

The goal is to expose practical operational functionality without trying to wrap the entire Firebird Services API in one phase.

## Version Target

Phase 06 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: backup helper on `FirebirdDatabase`

Implement:

- `BackupDatabase(backupFile As String) As Boolean`

This should run a server-side `gbak` backup through the Firebird service manager.

### Deliverable 2: restore helper on `FirebirdDatabase`

Implement:

- `RestoreDatabase(backupFile As String, targetDatabase As String, replaceExisting As Boolean) As Boolean`

This should run a server-side `gbak` restore through the Firebird service manager.

### Deliverable 3: service output access

Implement:

- `LastServiceOutput() As String`

This gives the caller the verbose `gbak` output from the last backup or restore request.

### Deliverable 4: desktop integration coverage

Add a desktop integration test that proves:

- backup succeeds
- the `.fbk` file is created
- restore succeeds
- the restored `.fdb` file can be opened
- a simple query on the restored database returns the expected result

## In Scope

- Firebird service manager attach/start/query/detach for backup and restore
- service-manager output capture using `isc_info_svc_line`
- Xojo public methods on `FirebirdDatabase`
- desktop integration coverage for backup and restore
- README and example updates

## Out of Scope

- user-management services
- trace services
- validation and repair services
- statistics services
- client-streamed backup or restore using `stdout` / `stdin`
- a standalone Xojo service-manager class

## Design Rules

### Rule 1: keep the public surface narrow

Phase 06 should add only the minimum useful operational methods instead of exposing raw service-manager primitives directly.

### Rule 2: reuse the active database identity

Backup and restore should use the current `FirebirdDatabase` connection settings for:

- host
- port
- user
- password
- expected database identity

### Rule 3: use official Firebird service-manager patterns

The implementation should follow Firebird's own installed examples and documentation for:

- attach SPB structure
- service start request structure
- verbose output polling

## Success Criteria

Phase 06 is complete when:

- `BackupDatabase(...)` works
- `RestoreDatabase(...)` works
- `LastServiceOutput()` returns the last verbose service output
- the desktop integration suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
