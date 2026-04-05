# Phase 04 Article

## Summary

Phase 04 added explicit Firebird transaction controls to the Xojo plugin.

This phase kept inherited `BeginTransaction` unchanged and added a separate Firebird-specific entry point for TPB-backed transaction start. The result is a small Xojo-facing API for isolation, access mode, and lock-timeout control that stays consistent with the Firebird engine and Jaybird-style semantics.

Final verification result on April 6, 2026:

- desktop integration suite: `96 passed, 0 failed`

Branch:

- `feature/phase-04`

## What Was Added

### 1. Low-level TPB-backed transaction start in `FBDatabase`

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Added:

- `beginTransactionWithOptions(...)`
- `setError(...)`

Implementation notes:

- the new path validates inputs before any Firebird API call
- isolation text is normalized into a stable internal form
- Firebird transaction parameter bytes are built explicitly
- default `beginTransaction()` remains untouched

Supported isolation inputs:

- `consistency`
- `concurrency`
- `snapshot`
- `read committed`
- `read committed record version`
- `read committed no record version`
- `read committed read consistency`

Lock-timeout behavior:

- `-1` uses `WAIT`
- `0` uses `NO WAIT`
- `> 0` uses `WAIT` plus `isc_tpb_lock_timeout`

### 2. Xojo-visible explicit transaction method

File:

- `sources/FirebirdPlugin.cpp`

Added public method:

- `BeginTransactionWithOptions(isolation As String, readOnly As Boolean, lockTimeout As Integer) As Boolean`

Behavior:

- returns `False` if the database is not connected
- returns `False` if a transaction is already active
- returns `False` for unsupported isolation text
- returns `True` only after Firebird accepts the TPB and starts the transaction
- switches the plugin into explicit-transaction mode by disabling auto-commit for that database handle

### 3. Desktop integration coverage

File:

- `example-desktop/MainWindow.xojo_window`

Added:

- `TestTransactionOptions`

The test validates:

- read-only explicit transaction start
- read-only explicit transaction info readback
- `NO WAIT` lock-timeout readback as `0`
- write rejection inside a read-only transaction
- read-write explicit transaction start
- explicit `concurrency` isolation readback
- positive lock-timeout readback
- invalid isolation rejection without opening a transaction

### 4. Expected read-only failure behavior documented in the test

The read-only-write check intentionally raises a `DatabaseException`.

Observed Firebird behavior:

- error code: `-817`
- message: `attempted update during read-only transaction`

This is not a Phase 04 defect. It is the proof that the read-only TPB was applied correctly.

When run in the Xojo debugger, this may surface as a handled exception before execution continues into the local `Catch ex As DatabaseException` block.

## Engine Validation Work

Before finalizing the implementation, the transaction parameter behavior was probed directly against the local Firebird client/runtime.

That validation confirmed:

- default explicit transactions on the current Firebird 5 setup use `concurrency`
- default access mode is `read write`
- default lock timeout is `-1`
- an explicit TPB with `read committed read consistency`, `read`, and `lock_timeout = 5` comes back through `isc_transaction_info` exactly as expected

This mattered because Phase 04 is built on Firebird engine truth first, with Jaybird as the semantic guideline.

## Docs and Examples Updated

Files:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_04.md`

Changes:

- README now documents explicit transaction options
- quick reference includes the new method and its intended usage
- API examples show a focused explicit-transaction example
- the master roadmap now reflects explicit transaction controls as complete in Phase 04

## What Phase 04 Deliberately Did Not Do

This phase did not add:

- savepoints
- transaction option objects or property-based configuration
- Services API
- Events API
- Array API
- interface-based API migration

That boundary was intentional. Phase 04 adds the minimum useful transaction-control surface without turning the plugin into a full Firebird administrative SDK.

## Outcome

Phase 04 closes the main transaction gap identified after Phase 03.

The plugin now supports:

- default explicit transactions through inherited `BeginTransaction`
- typed inspection of the active transaction
- explicit Firebird transaction control when isolation and wait behavior matter

The next logical step is outside core transaction setup:

- generated-key convenience decisions around `RETURNING`
- Services API scope for backup, restore, and user-management workflows
- savepoint scope decision, if it proves necessary for the Xojo surface
