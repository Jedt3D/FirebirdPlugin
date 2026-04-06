# Phase 16 Article

## Summary

Phase 16 continues the maintenance/repair track and adds explicit limbo-recovery helpers to `FirebirdDatabase`:

- `CommitLimboTransaction(transactionId As Int64) As Boolean`
- `RollbackLimboTransaction(transactionId As Int64) As Boolean`

This extends the repair-oriented service surface from limbo inspection into targeted recovery without mixing in shutdown or broader repair workflows.

## What Was Added

### 1. Limbo-recovery helpers

The new methods use the Firebird service manager with:

- `isc_action_svc_repair`
- `isc_spb_dbname`
- `isc_spb_rpr_commit_trans_64`
- `isc_spb_rpr_rollback_trans_64`

Both methods take an explicit transaction id and send a single-target recovery request through the same utility-interface SPB builder pattern already used by the existing services slices.

### 2. Desktop integration coverage

The desktop suite gained `TestRecoverLimboTransactions`, which verifies:

- the recovery methods are exposed to Xojo
- a clean database can execute the recovery path against a known non-limbo transaction id
- the engine response remains safe whether it rejects that id or accepts it as a no-op
- limbo listing is still clean afterward
- real limbo transactions, if unexpectedly present, are not auto-committed or auto-rolled back by the test harness

## Important Behavioral Note

The first version of the Phase 16 test assumed Firebird would reject recovery requests for a non-limbo transaction id. On this environment, the engine accepted those requests as a no-op instead.

That means the portable contract for this plugin is not "non-limbo ids must fail". The stable contract is:

- recovery requests can be issued safely
- service output may be empty
- a clean database remains clean after the request

The final test was updated to verify that engine-safe behavior instead of forcing one specific response mode.

## Why This Slice Was Chosen

After Phase 14 added read-only limbo listing, the next logical repair step was targeted recovery by transaction id. This slice was chosen because it:

- builds directly on the existing limbo service path
- adds real administrative capability without taking the database offline
- keeps the API narrow and transaction-targeted
- avoids the higher operational risk of shutdown/online workflows in the same phase

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
- `phase_16.md`

## Final Result

Phase 16 closed with the desktop suite green:

- `149 passed`
- `0 failed`

Verified on April 6, 2026.
