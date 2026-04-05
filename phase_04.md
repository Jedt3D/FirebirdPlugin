# Phase 04 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `96 passed, 0 failed`
- article: `phase_04_article.md`

Branch:

- `feature/phase-04`

## Objective

Phase 04 adds explicit Firebird transaction controls to the Xojo plugin while preserving the existing default `BeginTransaction` behavior.

The goal is to expose a small, stable TPB-backed entry point that follows Jaybird-style transaction semantics closely enough for Firebird 4/5/6 work, without redesigning the entire Xojo `Database` abstraction.

## Version Target

Phase 04 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: Explicit TPB-backed transaction start

Add a new low-level transaction-start path in `FBDatabase` that can:

- choose isolation mode
- choose read-only vs read-write access
- choose wait behavior through lock timeout

### Deliverable 2: Xojo-visible explicit transaction method

Expose a Firebird-specific method on `FirebirdDatabase`:

- `BeginTransactionWithOptions(isolation As String, readOnly As Boolean, lockTimeout As Integer) As Boolean`

Initial supported isolation strings:

- `consistency`
- `concurrency`
- `snapshot`
- `read committed`
- `read committed record version`
- `read committed no record version`
- `read committed read consistency`

Lock-timeout semantics:

- `-1` = wait indefinitely
- `0` = `NO WAIT`
- `> 0` = wait for the specified number of seconds

### Deliverable 3: Desktop integration coverage

Add a desktop test that proves:

- explicit read-only and read-write transactions can be started successfully
- the transaction info helpers reflect the chosen TPB settings
- a read-only explicit transaction rejects writes
- invalid isolation text is rejected cleanly without opening a transaction

## In Scope

- explicit TPB generation for the supported isolation modes
- read-only / read-write control
- lock-timeout control
- Xojo-visible method exposure
- desktop integration coverage
- README and example updates

## Out of Scope

- savepoints
- named transaction parameter objects
- full Jaybird transaction-parameter parity
- Services API
- Events API
- Array API
- interface-based API migration

## Design Rules

### Rule 1: Do not change inherited `BeginTransaction`

The existing `BeginTransaction` path remains the plugin default.

Phase 04 adds an explicit Firebird-only alternative instead of redefining inherited behavior.

### Rule 2: Keep the public surface small

Do not expose raw TPB bytes or a large configuration object in this phase.

Use one narrow method with normalized string inputs first.

### Rule 3: Reject ambiguous situations explicitly

If a transaction is already active, the explicit transaction method should fail instead of silently reusing it with unknown settings.

## Success Criteria

Phase 04 is complete when:

- explicit transaction options are available from Xojo
- the desktop test suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
