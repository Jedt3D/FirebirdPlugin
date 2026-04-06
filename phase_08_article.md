# Phase 08 Article

## Summary

Phase 08 adds a narrow, non-destructive validation slice to the Firebird Xojo plugin:

- `ValidateDatabase() As Boolean`
- reuse of `LastServiceOutput() As String` for validation diagnostics

The scope stays intentionally small. This phase does not expose repair, mend, sweep, or other destructive maintenance operations.

## What Was Implemented

### 1. Xojo-facing validation method

`FirebirdDatabase` now exposes:

- `ValidateDatabase() As Boolean`

This gives the plugin a simple operational validation entry point that matches the earlier service-manager style introduced in Phases 06 and 07.

### 2. Native Firebird service-manager integration

The native implementation was added in the C++ layer and routed through the existing service-manager plumbing:

- service attach
- service start
- service query
- service detach

Validation output is captured into the existing `LastServiceOutput()` path, so the Xojo surface stays consistent across:

- backup
- restore
- statistics
- validation

### 3. Correct online-validation behavior

The initial implementation attempt used the repair action path, which failed on a live attached database with Firebird reporting that secondary attachments cannot validate databases through that route.

Phase 08 was corrected to use Firebird's online validation service action:

- `isc_action_svc_validate`

That is the important implementation detail for this phase. The result is a validation path that works with the normal connected-database workflow used by this plugin and its desktop integration tests.

### 4. Desktop integration coverage

A new desktop test was added:

- `TestValidateDatabase`

The test verifies:

- `ValidateDatabase()` succeeds
- `LastServiceOutput()` remains usable after validation
- validation content is accepted both when Firebird produces explicit messages and when a clean database produces minimal output

## Files Changed

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`
- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_08.md`

## Result

Phase 08 completed with the desktop suite green:

- `113 passed, 0 failed`

## Outcome for the Roadmap

After Phase 08, the plugin's first Services API slice now includes:

- backup
- restore
- database statistics
- online validation
- service output capture

The next logical services step is no longer basic validation. The next narrow slice should be either:

- user-management workflows
- a more selective maintenance workflow beyond validation
