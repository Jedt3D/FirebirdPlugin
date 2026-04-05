# Phase 03 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `86 passed, 0 failed`
- article: `phase_03_article.md`

Branch:

- `feature/phase-03`

## Objective

Phase 03 adds transaction-info visibility to the existing Firebird Xojo plugin without changing transaction-start semantics yet.

The goal is to expose the most useful `isc_transaction_info` values through typed Xojo methods so users can inspect the current transaction state and so the plugin has a clean base for later transaction-control work.

## Version Target

Phase 03 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: Low-level transaction-info wrapper

Add `isc_transaction_info` support in `FBDatabase`.

Initial scope:

- transaction id
- transaction isolation
- transaction access mode
- lock timeout

Keep the API typed and small. Do not expose raw info buffers to Xojo.

### Deliverable 2: Xojo-visible transaction state helpers

Expose Firebird-specific helpers on `FirebirdDatabase`.

Target methods:

- `HasActiveTransaction() As Boolean`
- `TransactionID() As Int64`
- `TransactionIsolation() As String`
- `TransactionAccessMode() As String`
- `TransactionLockTimeout() As Integer`

Behavior when no transaction is active:

- `HasActiveTransaction()` returns `False`
- `TransactionID()` returns `0`
- `TransactionLockTimeout()` returns `-2`
- string methods return `""`

Lock-timeout semantics:

- `-1` means the current transaction waits indefinitely
- `-2` means the value is unavailable because there is no active transaction or the info call failed

### Deliverable 3: Desktop integration coverage

Add a local desktop test that proves:

- explicit transactions report as active after `BeginTransaction`
- a live transaction exposes a non-zero transaction id
- isolation and access mode are readable
- transaction state resets after commit

## In Scope

- `isc_transaction_info` wrapper code
- typed transaction-info parsing
- Xojo-visible transaction-info methods
- desktop integration coverage
- README and example updates if the public API changes are user-relevant enough to document now

## Out of Scope

- transaction parameter buffer redesign
- configurable isolation levels
- savepoints
- Services API
- Events API
- Array API
- interface-based API migration

## Design Rules

### Rule 1: Keep transaction start semantics unchanged

Phase 03 observes transactions; it does not redesign how transactions are started.

That means:

- `BeginTransaction` still uses the current default Firebird behavior
- no TPB customization is introduced in this phase

### Rule 2: Return normalized strings for semantic values

For Xojo-facing methods, use normalized strings instead of raw numeric constants.

Examples:

- `concurrency`
- `read committed record version`
- `read committed no record version`
- `read committed read consistency`
- `read write`
- `read only`

### Rule 3: Avoid raising complexity for auto-commit paths

The helpers should inspect the currently active transaction only.

They must not:

- auto-start a transaction just to answer an info request
- change auto-commit behavior
- keep transactions open longer than the current plugin already does

## Internal Work Plan

### Step 1: Add low-level transaction-info helpers

File targets:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Work:

- add a reusable `transactionInfo(...)` helper
- add typed helpers for id, isolation, access mode, and lock timeout
- parse Firebird transaction-info responses into stable Xojo-facing values

### Step 2: Expose Xojo methods

File target:

- `sources/FirebirdPlugin.cpp`

Work:

- add the Phase 03 methods to `FirebirdDatabase`
- preserve the current `Database` subclass shape
- return safe defaults when no explicit transaction is active

### Step 3: Add desktop test coverage

File target:

- `example-desktop/MainWindow.xojo_window`

Initial test name:

- `TestTransactionInfo`

## Success Criteria

Phase 03 is complete when:

- transaction info is readable from Xojo during an explicit transaction
- the desktop suite passes with the new transaction-info test included
- docs and the detailed phase article are written
