# Phase 14 Article

## Summary

Phase 14 continues the maintenance/services track and adds a read-only limbo-transaction helper to `FirebirdDatabase`:

- `ListLimboTransactions() As Boolean`

This extends the repair-oriented service coverage without introducing destructive recovery actions.

## What Was Added

### 1. Limbo-transaction listing helper

`ListLimboTransactions()` uses the Firebird service manager with:

- `isc_action_svc_repair`
- `isc_spb_dbname`
- `isc_spb_options = isc_spb_rpr_list_limbo_trans`

The method follows the plugin's established service-manager lifecycle:

- attach to service manager
- start the service action
- query service output
- detach cleanly

### 2. Desktop integration coverage

The desktop suite gained `TestListLimboTransactions`, which verifies:

- the limbo-list action succeeds
- service-output handling remains stable
- a clean database can produce either diagnostic output or no output at all without being treated as failure

This keeps the verification realistic without trying to synthesize a true limbo transaction in the local desktop suite.

## Why This Slice Was Chosen

After Phase 13's sweep support, the next safest maintenance step was another read-only repair action. Listing limbo transactions fits that goal because it:

- uses the same service-manager repair family already in place
- does not mutate database state on the success path
- is practical to verify against a normal development database

It also lays groundwork for any future limbo-recovery slice without forcing recovery APIs into the plugin yet.

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
- `phase_14.md`

## Final Result

Phase 14 closed with the desktop suite green:

- `141 passed`
- `0 failed`

Verified on April 6, 2026.
