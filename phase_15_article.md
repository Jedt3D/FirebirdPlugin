# Phase 15 Article

## Summary

Phase 15 continues the maintenance/properties track and adds a reversible sweep-interval helper to `FirebirdDatabase`:

- `SetSweepInterval(interval As Integer) As Boolean`

This extends the service-manager surface with a database-properties action that is easy to verify and safe to restore in the desktop suite.

## What Was Added

### 1. Sweep-interval property helper

`SetSweepInterval()` uses the Firebird service manager with:

- `isc_action_svc_properties`
- `isc_spb_dbname`
- `isc_spb_prp_sweep_interval`

The method validates that a database path is available, rejects negative intervals up front, and then runs the database-properties action through the same utility-interface SPB builder used in the existing services slices.

### 2. Desktop integration coverage

The desktop suite gained `TestSetSweepInterval`, which verifies:

- the property update succeeds
- the service-output path remains stable when the engine emits no verbose text
- the new value is visible through `MON$DATABASE.MON$SWEEP_INTERVAL`
- the original value is restored after the test

Using `MON$DATABASE` for readback keeps the verification direct and avoids depending on formatted service output.

## Why This Slice Was Chosen

After the sweep and limbo-list helpers, the next low-risk maintenance step was a reversible database-properties action. Sweep interval fits that goal because it:

- uses the same service-manager family already in place
- changes a single well-defined database property
- can be verified with a direct metadata query
- can be restored immediately in the same test run

That makes it a better incremental step than shutdown or recovery actions, which are more disruptive and harder to automate safely in the desktop suite.

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
- `phase_15.md`

## Final Result

Phase 15 closed with the desktop suite green:

- `144 passed`
- `0 failed`

Verified on April 6, 2026.
