# Phase 24 Detailed Plan

Status: Complete on April 7, 2026  
Branch: `feature/phase-24`

## Goal

Evaluate whether Firebird events can be exposed as a useful Xojo API with PostgreSQL as the parity reference, and decide whether the next step should be implementation or further deferral.

## PostgreSQL Parity Target

The relevant Xojo `PostgreSQLDatabase` surface is:

- `Listen(name As String)`
- `StopListening(name As String)`
- `Notify(name As String)`
- `CheckForNotifications()`
- `ReceivedNotification(Name As String, ID As Integer, Extra As String)`

The question for Phase 24 is not whether Firebird supports events at all, but whether these semantics can be mapped honestly and usefully.

## Firebird Capability Findings

Firebird exposes two event-listening models in the client API:

- synchronous `isc_wait_for_event()`
- asynchronous `isc_que_events()` / `isc_cancel_events()` with `isc_event_block()` and `isc_event_counts()`

Important behavior:

1. Firebird notifications are event-name based.
2. Notifications are generated through `POST_EVENT` in PSQL.
3. Event delivery occurs when the posting transaction commits.
4. Asynchronous notification is callback-based and must be re-queued after each delivery.
5. Event results provide event names and occurrence counts, not PostgreSQL-style sender IDs or payload strings.

## Design Decision

Phase 24 closes with a go decision on limited event support.

That means Firebird events are worth exposing in the public Xojo surface, but not as a literal copy of PostgreSQL semantics.

## Recommended Public Shape

The cleanest next implementation target is:

- `Listen(name As String) As Boolean`
- `StopListening(name As String) As Boolean`
- `CheckForNotifications() As Boolean`
- `Notify(name As String) As Boolean`

And one Firebird-specific event:

- `ReceivedNotification(Name As String, Count As Integer)`

This differs from PostgreSQL in two intentional ways:

1. no sender process ID
2. no payload / extra string

Reason:

- Firebird events naturally expose name and count
- pretending otherwise would manufacture semantics the engine does not provide

## Implementation Strategy

If implemented in the next phase, it should use:

1. `isc_event_block()` to register the event set
2. `isc_que_events()` for asynchronous listening
3. `isc_event_counts()` to calculate per-event counts
4. automatic re-queueing after callback delivery
5. `CheckForNotifications()` as the Xojo-side queue drain and dispatch point

Why not use `isc_wait_for_event()`:

- it is blocking
- it is a poor fit for desktop UI responsiveness
- it does not match PostgreSQL's timer-driven `CheckForNotifications()` pattern as well as a queued async design does

## Notify Mapping

Firebird does not have a direct client-side `Notify` protocol command like PostgreSQL.

The closest honest mapping is:

- implement `Notify(name)` as a convenience wrapper over a small PSQL `EXECUTE BLOCK` that calls `POST_EVENT`

That is acceptable if documented clearly:

- delivery still happens on commit
- it is a Firebird event-posting wrapper, not a wire-protocol analogue of PostgreSQL `NOTIFY`

## Remaining Risk

The main remaining implementation risk is not engine capability. It is Xojo plugin surface wiring:

- maintaining callback state safely
- queueing notifications across callback boundaries
- deciding whether `ReceivedNotification` should be a true plugin event or whether `CheckForNotifications()` should return queued data directly

This is a manageable implementation risk and a better fit than the large-object parity effort from Phase 23.

## Practical Recommendation

Phase 24 completes as a design / feasibility slice with a positive implementation recommendation.

Recommended next step:

- implement a limited Firebird event API in Phase 25

Recommended Phase 25 scope:

- `Listen`
- `StopListening`
- `CheckForNotifications`
- `Notify`
- one Firebird-specific notification event carrying `Name` and `Count`

## Source Basis

This phase was based on:

- local Xojo documentation for `PostgreSQLDatabase` notification APIs
- Firebird `ibase.h`
- official Firebird documentation for `POST_EVENT`
- official Firebird driver/API documentation describing `isc_que_events()`, callback re-queueing, and event counts
