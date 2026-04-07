# Phase 25 Article

Phase 25 takes the positive design decision from Phase 24 and turns it into a real Xojo feature.

The Firebird plugin now supports a limited event API:

- `Listen`
- `StopListening`
- `CheckForNotifications`
- `Notify`
- `ReceivedNotification(name, count)`

## Why This Matters

This is one of the clearer places where Xojo's built-in PostgreSQL driver sets user expectations. Developers expect a familiar pattern for database notifications:

- subscribe
- post
- poll
- receive an event

Firebird can support that pattern well enough, but only if the plugin stays honest about Firebird semantics.

## What the Plugin Does

Internally, the plugin uses Firebird's asynchronous event API:

- `isc_event_block`
- `isc_que_events`
- `isc_event_counts`
- `isc_cancel_events`

The Firebird callback does not call into Xojo directly. It only queues notifications. The actual Xojo event is emitted from `CheckForNotifications()`.

That choice matters because it avoids crossing from Firebird's callback thread directly into Xojo runtime code.

## What It Does Not Pretend To Be

This is not a literal clone of PostgreSQL notifications.

PostgreSQL gives you:

- a name
- a sender ID
- an optional payload

Firebird gives us cleanly:

- an event name
- an occurrence count

So the plugin surface reflects that:

- `ReceivedNotification(name As String, count As Integer)`

That is the right compromise. It is familiar enough for Xojo developers, but it does not invent behavior the engine does not actually provide.

## Notify

`Notify(name)` is implemented as a convenience wrapper over `POST_EVENT` through `EXECUTE BLOCK`.

So this is still Firebird's event model:

- delivery occurs on commit
- posting is transactional

## Result

Phase 25 closes one of the most visible PostgreSQL-parity gaps with a practical, shipped API.

The Firebird plugin now feels closer to a first-class built-in driver in another area that real Xojo apps notice, while still preserving Firebird's actual engine semantics.
