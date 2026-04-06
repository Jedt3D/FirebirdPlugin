# Phase 13 Article

## Summary

Phase 13 returns to the maintenance/services track and adds one narrow operational helper to `FirebirdDatabase`:

- `SweepDatabase() As Boolean`

This exposes Firebird's service-manager sweep action without opening a larger repair or shutdown surface.

## What Was Added

### 1. Database sweep helper

`SweepDatabase()` uses the Firebird service manager with:

- `isc_action_svc_repair`
- `isc_spb_dbname`
- `isc_spb_options = isc_spb_rpr_sweep_db`

The method follows the same service-manager lifecycle already used by the plugin's existing operational helpers:

- attach to service manager
- start the service action
- query service output
- detach cleanly

### 2. Desktop integration coverage

The desktop suite gained `TestSweepDatabase`, which verifies:

- the sweep action succeeds
- service output handling remains stable even when sweep emits no verbose text
- the database is still queryable immediately afterward

The post-sweep verification uses a normal SQL query against `artists`, which keeps the test non-destructive and easy to repeat.

## Mid-Phase Fix

The first sweep implementation failed with:

- `Invalid clumplet buffer structure: unknown parameter for repair (107)`

That error identified `107` as `isc_spb_verbose`. Unlike backup/restore, the repair/sweep request does not accept that parameter in this context. Removing `isc_spb_verbose` from the sweep request fixed the service call.

This is why the final test accepts both outcomes for service output:

- non-empty output
- or a successful sweep with no verbose output

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `phase_13.md`

## Final Result

Phase 13 closed with the desktop suite green:

- `138 passed`
- `0 failed`

Verified on April 6, 2026.
