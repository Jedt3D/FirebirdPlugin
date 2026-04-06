# Phase 07 Article

## Summary

Phase 07 extended the first Services API slice with database statistics.

Phase 06 established service-manager backup and restore. Phase 07 builds on that same service-manager foundation and adds a single new operational capability:

- Firebird database statistics through `DatabaseStatistics()`

Final verification result on April 6, 2026:

- desktop integration suite: `110 passed, 0 failed`

Branch:

- `feature/phase-07`

## What Was Added

### 1. `DatabaseStatistics()` on `FirebirdDatabase`

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Added Xojo-visible method:

- `DatabaseStatistics() As Boolean`

This runs Firebird database statistics for the currently connected database through the service manager and stores the textual report in:

- `LastServiceOutput() As String`

### 2. Statistics request built on the Phase 06 service-manager path

Files:

- `sources/FirebirdDB.cpp`

The implementation reuses the existing service-manager execution flow from Phase 06:

- attach to `service_mgr`
- start a service request
- poll line-based output with `isc_info_svc_line`
- detach cleanly

Phase 07 adds the statistics-specific start request:

- `isc_action_svc_db_stats`
- current database name/path
- optional SQL role
- a narrow page-analysis option set

### 3. Correct statistics option selection

File:

- `sources/FirebirdDB.cpp`

The first Phase 07 attempt mixed header-page mode with page-analysis mode and failed with:

- `option -h is incompatible with options -a, -d, -i, -r, -s and -t`

That was a real Firebird `gstat` option conflict.

The fix was to narrow the default request to a compatible statistics set:

- `isc_spb_sts_data_pages`
- `isc_spb_sts_idx_pages`

This keeps the method useful while avoiding an overly broad or conflicting default report mode.

### 4. Desktop integration coverage

File:

- `example-desktop/MainWindow.xojo_window`

Added test:

- `TestDatabaseStatistics`

The test proves:

- `DatabaseStatistics()` succeeds
- `LastServiceOutput()` is not empty
- the report contains expected `gstat` output markers

This gives real end-to-end coverage for the second operational service-manager workflow.

### 5. Public docs and examples

Files:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_07.md`

Changes:

- README now documents the statistics method alongside backup and restore
- quick reference includes the statistics workflow
- API examples now show backup, restore, and statistics together
- the master plan now marks the first Services API slice as backup + restore + statistics

## What Phase 07 Deliberately Did Not Do

This phase did not add:

- configurable statistics flags
- per-table statistics targeting
- validation and repair services
- user-management services
- trace services
- a generic service-manager abstraction

That boundary remains intentional. Phase 07 adds one more concrete operational feature without turning the plugin into an unbounded admin wrapper.

## Important Runtime Notes

- statistics runs against the currently connected database identity
- `LastServiceOutput()` is reused consistently for backup, restore, and statistics
- the report shape is intentionally narrow and uses a compatible `gstat` option set

## Outcome

Phase 07 completes the first practical service-manager bundle:

- backup
- restore
- database statistics
- verbose output capture

The next best step is no longer generic service-manager plumbing. The next best step is choosing the next narrow operational slice:

- validation / repair
- user-management workflows

and continuing the same pattern of:

- one small typed method
- one end-to-end desktop test
- one phase article and branch commit
