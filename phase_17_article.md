# Phase 17 Article

## Summary

Phase 17 continues the maintenance/services track and adds controlled availability helpers to `FirebirdDatabase`:

- `ShutdownDenyNewAttachments(timeoutSeconds As Integer) As Boolean`
- `BringDatabaseOnline() As Boolean`

This extends the operational surface from repair and property changes into reversible database availability control.

## What Was Added

### 1. Shutdown / online helpers

The new methods use the Firebird service manager with:

- `isc_action_svc_properties`
- `isc_spb_prp_shutdown_mode`
- `isc_spb_prp_attachments_shutdown`
- `isc_spb_prp_online_mode`

The shutdown path uses denied new attachments in multi-user shutdown mode. The online path restores normal attachment behavior.

### 2. Desktop integration coverage

The desktop suite gained `TestShutdownOnlineControl`, which verifies:

- the shutdown and online methods are exposed to Xojo
- an ordinary user can connect before shutdown
- new ordinary-user attachments are denied while shutdown is active
- the online helper restores ordinary-user attachment success
- cleanup always biases toward bringing the database back online

## Important Implementation Note

The first implementation tried to issue shutdown and online requests through the same live `FirebirdDatabase` instance used for SQL work. That produced Firebird's engine-level complaint:

- `database shutdown unsuccessful`
- `Secondary attachment - config data from DPB ignored`

The stable shape for this feature is:

- keep SQL work on a normal connected `FirebirdDatabase`
- create a separate service-control `FirebirdDatabase` object for shutdown/online
- do not use that control object for SQL statements

The final wrappers build that service-only context internally, and the desktop test follows the same separation.

## Why This Slice Was Chosen

After Phase 16 completed limbo recovery, the next high-value maintenance step was controlled availability management. This slice was chosen because it:

- extends the service-manager surface in a practical way
- stays reversible
- avoids broader shutdown-mode complexity
- creates a usable foundation for later operational/admin work

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`
- `Makefile`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `phase_17.md`

## Final Result

Phase 17 closed with the desktop suite green:

- `157 passed`
- `0 failed`

Verified on April 6, 2026.
