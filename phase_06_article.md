# Phase 06 Article

## Summary

Phase 06 added the first operational Services API slice to the Xojo Firebird plugin.

This phase intentionally did not try to expose the full Firebird service manager. It focused on the most immediately useful workflow:

- server-side backup
- server-side restore
- access to the verbose `gbak` output from the last service operation

Final verification result on April 6, 2026:

- desktop integration suite: `107 passed, 0 failed`

Branch:

- `feature/phase-06`

## What Was Added

### 1. Service-manager backup and restore on `FirebirdDatabase`

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Added Xojo-visible methods:

- `BackupDatabase(backupFile As String) As Boolean`
- `RestoreDatabase(backupFile As String, targetDatabase As String, replaceExisting As Boolean) As Boolean`
- `LastServiceOutput() As String`

These methods run Firebird's server-side `gbak` operations through the service manager instead of shelling out to a separate utility process.

### 2. Service-manager request execution in the native wrapper

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Added native support for:

- attaching to `service_mgr`
- starting service requests
- polling verbose output line-by-line
- detaching cleanly from the service manager

The implementation reuses the connected database identity:

- host
- port
- user
- password
- role
- expected database path

That keeps the public Xojo API narrow and avoids inventing a separate service-connection object in this phase.

### 3. Official Firebird SPB handling for restore correctness

File:

- `sources/FirebirdDB.cpp`

During Phase 06 implementation, backup succeeded first but restore initially failed with:

- `Invalid clumplet buffer structure: unknown parameter for backup/restore (0)`

The fix was to stop hand-encoding the `SPB_START` request for backup and restore and instead build it with Firebird's own `IXpbBuilder`.

That change matters because:

- service-manager attach SPBs and start SPBs are not identical
- integer-valued clumplets such as `isc_spb_options` are easy to encode incorrectly by hand
- Firebird's own builder guarantees the correct binary layout for the service request

The final working design is:

- attach SPB built explicitly for `isc_service_attach`
- start SPB for backup/restore built through `IXpbBuilder`
- verbose output drained through repeated `isc_info_svc_line` queries

### 4. Desktop integration coverage

File:

- `example-desktop/MainWindow.xojo_window`

Added test:

- `TestServicesBackupRestore`

The test proves:

- `BackupDatabase(...)` succeeds
- the backup file is created
- `LastServiceOutput()` contains verbose backup output
- `RestoreDatabase(...)` succeeds
- the restored database file is created
- `LastServiceOutput()` contains verbose restore output
- the restored database can be opened and queried successfully

This gives the first real end-to-end operational coverage beyond normal SQL execution.

### 5. Public docs and examples

Files:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_06.md`

Changes:

- README now documents the backup/restore API and `LastServiceOutput()`
- quick reference includes the new Services API first slice
- API examples now include a focused backup/restore example
- the master plan now marks Services API as partially implemented instead of missing

## What Phase 06 Deliberately Did Not Do

This phase did not add:

- user-management services
- statistics services
- validation and repair services
- trace services
- client-streamed backup/restore using `stdout` / `stdin`
- a standalone Xojo service-manager class

That boundary was intentional. Backup and restore are the most immediately valuable operational slice, and they establish the service-manager plumbing without overcommitting the public API too early.

## Important Runtime Notes

- backup and restore are server-side operations, so the file paths must be valid from the Firebird server's point of view
- `LastServiceOutput()` returns the verbose `gbak` output from the most recent backup or restore
- the Phase 04 read-only transaction test still raises Firebird error `-817` as an expected handled pass condition

## Outcome

Phase 06 closes the first major operational gap in the plugin.

The plugin now covers:

- core SQL execution
- metadata helpers
- explicit transaction controls
- generated-key convenience through Xojo's native insert hook
- backup and restore through the Firebird service manager

The next best step is not “more generic Services API” in the abstract. The next best step is choosing the next concrete operational slice:

- statistics
- validation / repair
- user-management workflows

and keeping the public Xojo surface narrow and typed as each slice is added.
