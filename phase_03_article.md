# Phase 03 Article

## Summary

Phase 03 added transaction-info visibility to the Firebird Xojo plugin.

This phase did not redesign transaction creation or isolation control. It exposed a small typed inspection surface over Firebird's legacy `isc_transaction_info` API so Xojo code can inspect the currently active explicit transaction.

Final verification result on April 6, 2026:

- desktop integration suite: `86 passed, 0 failed`

Branch:

- `feature/phase-03`

## What Was Added

### 1. Low-level transaction-info support in `FBDatabase`

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Added:

- `transactionInfo(...)`
- `transactionID(...)`
- `transactionIsolation(...)`
- `transactionAccessMode(...)`
- `transactionLockTimeout(...)`

Implementation notes:

- the code uses `isc_transaction_info`
- it follows the same pattern as the existing database-info helpers
- it does not auto-start a transaction
- it only inspects `mTrans` when a transaction is already active

The typed helpers parse these Firebird info items:

- `isc_info_tra_id`
- `isc_info_tra_isolation`
- `isc_info_tra_access`
- `isc_info_tra_lock_timeout`

### 2. Xojo-visible transaction-info helpers on `FirebirdDatabase`

File:

- `sources/FirebirdPlugin.cpp`

Added public methods:

- `HasActiveTransaction() As Boolean`
- `TransactionID() As Int64`
- `TransactionIsolation() As String`
- `TransactionAccessMode() As String`
- `TransactionLockTimeout() As Integer`

Behavior:

- no active transaction:
  - `HasActiveTransaction = False`
  - `TransactionID = 0`
  - `TransactionIsolation = ""`
  - `TransactionAccessMode = ""`
  - `TransactionLockTimeout = -2`
- active transaction:
  - transaction id comes back as `Int64`
  - isolation and access mode are normalized to readable strings
  - lock timeout preserves Firebird semantics

### 3. Normalized semantic values

Phase 03 returns string values instead of raw Firebird constants.

Examples:

- `concurrency`
- `read committed record version`
- `read committed no record version`
- `read committed read consistency`
- `read write`
- `read only`

This keeps the Xojo surface practical while staying faithful to Firebird.

### 4. Correct Xojo integer bridging for lock timeout

One follow-up fix was required after the first test run.

The initial implementation used a 32-bit return type for `TransactionLockTimeout() As Integer`. On this machine, Firebird reports the default lock timeout as `-1` for "wait indefinitely", but Xojo received it as `4294967295`.

The fix was:

- change the plugin implementation to use `RBInteger` for `As Integer` methods where signed platform-sized semantics matter

After that fix:

- `TransactionLockTimeout = -1` correctly reaches Xojo

## Test Coverage Added

File:

- `example-desktop/MainWindow.xojo_window`

Added:

- `TestTransactionInfo`

The test validates:

- no active transaction before `BeginTransaction`
- `TransactionID = 0` when no transaction exists
- transaction becomes active after `BeginTransaction`
- transaction id is non-zero
- isolation is readable
- access mode is readable
- lock timeout is readable
- transaction state clears after commit

Observed runtime behavior on the current Firebird 5 environment:

- default explicit transaction isolation: `concurrency`
- default access mode: `read write`
- default lock timeout: `-1`

## Docs and Examples Updated

Files:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_03.md`

Changes:

- README now documents the transaction-info helper surface
- quick reference includes the new Firebird-only methods
- API examples show transaction inspection in the metadata example
- the master roadmap now reflects:
  - Phase 01 complete
  - Phase 02 complete
  - Phase 03 transaction inspection complete

## What Phase 03 Deliberately Did Not Do

This phase did not change:

- transaction parameter buffer handling
- configurable isolation levels
- savepoints
- Services API
- Events API
- Array API
- interface-based API migration

That boundary was intentional. Phase 03 was a visibility phase, not a transaction redesign phase.

## Outcome

Phase 03 leaves the plugin in a better position for the next development step:

- users can inspect the current transaction from Xojo
- the plugin now covers both database info and transaction info at a typed helper level
- later transaction-control work can build on verified runtime semantics instead of assumptions

The next logical step is richer transaction control guided by Jaybird:

- configurable TPB behavior
- isolation selection
- read-only vs read-write selection
- possibly lock-timeout configuration
