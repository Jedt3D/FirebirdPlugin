# Phase 25 Detailed Plan

Status: Complete on April 7, 2026  
Branch: `feature/phase-25`

## Goal

Implement the limited Firebird event API approved in Phase 24, using a Xojo-facing method pattern that feels familiar to `PostgreSQLDatabase` without pretending Firebird exposes PostgreSQL payload semantics.

## Public Xojo Surface

Phase 25 adds these methods on `FirebirdDatabase`:

- `Listen(name As String)`
- `StopListening(name As String)`
- `CheckForNotifications()`
- `Notify(name As String)`

And one new event:

- `ReceivedNotification(name As String, count As Integer)`

This is intentionally narrower than Xojo's PostgreSQL event surface:

- no sender process ID
- no payload / extra string

Reason:

- Firebird naturally exposes event names and occurrence counts
- fabricating PostgreSQL-style payload semantics would be misleading

## Implementation Strategy

The implementation uses Firebird's asynchronous event queue:

1. `isc_event_block()` builds the event registration buffer
2. `isc_que_events()` arms asynchronous listening
3. `isc_event_counts()` resolves the delivered event-name counts
4. `isc_cancel_events()` shuts the listener down safely
5. the callback automatically re-queues the event registration

Important runtime choice:

- the Firebird callback only queues notifications inside plugin-managed state
- the Xojo event is raised from `CheckForNotifications()`

That keeps Xojo runtime calls off the Firebird callback thread and matches the timer/polling usage model documented for `PostgreSQLDatabase.CheckForNotifications`.

## Notify Semantics

Firebird does not expose a direct client-side `NOTIFY` command.

`Notify(name)` is implemented as a convenience wrapper over:

- `EXECUTE BLOCK AS BEGIN POST_EVENT 'name'; END`

This means:

- event delivery still occurs on commit
- the method is a Firebird event-posting wrapper, not a PostgreSQL wire-protocol equivalent

## Windows ARM64 Support

Phase 25 also extends the Windows ARM64 dynamic-loader surface to include the Firebird event entry points:

- `isc_cancel_events`
- `isc_que_events`
- `isc_event_block`
- `isc_event_counts`
- `isc_free`

That keeps the new API from becoming macOS-only by accident.

## Test Scope

Desktop verification covers:

1. `Listen`
2. `Notify`
3. `ReceivedNotification`
4. `StopListening`
5. confirming no further notifications arrive after `StopListening`

The test uses:

- one listener connection
- one sender connection
- explicit `CheckForNotifications()` polling

## Result

Phase 25 closes successfully as a shipped implementation slice.

Verified suite result:

- `180 passed`
- `0 failed`

## Practical Outcome

The Firebird plugin now closes the most important PostgreSQL notification-parity gap with a Firebird-native event model that is:

- honest
- testable
- safe with Xojo's runtime threading assumptions

The next parity decision should move to the remaining advanced gap:

- a Firebird-native streaming BLOB handle only if blob-id ergonomics can be exposed cleanly
