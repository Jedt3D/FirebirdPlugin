# Phase 24 Article

Phase 24 answers the question that Phase 23 left open: is there a PostgreSQL-style parity path that actually makes sense for Firebird?

For events, the answer is yes, but only if the API stays honest about Firebird semantics.

## What Firebird Gives Us

Firebird already has a real event mechanism:

- applications can register interest in named events
- server-side code can post events with `POST_EVENT`
- the client API can receive asynchronous callbacks
- the callback payload naturally resolves to event names plus occurrence counts

So unlike the large-object parity question, this is not blocked by a weak engine fit.

## Where It Differs From PostgreSQL

PostgreSQL notifications expose:

- a name
- a sender ID
- an extra payload string

Firebird does not naturally expose that same shape. What it gives us cleanly is:

- event name
- occurrence count

That means direct parity should stop at the method pattern, not the full payload signature.

## Recommended Xojo Surface

The best next API is:

- `Listen(name As String)`
- `StopListening(name As String)`
- `CheckForNotifications()`
- `Notify(name As String)`

With a Firebird-specific callback or event such as:

- `ReceivedNotification(Name As String, Count As Integer)`

This is close enough to PostgreSQL to feel familiar, but still honest about what Firebird actually provides.

## Important Implementation Choice

The implementation should be built on the asynchronous event queue:

- `isc_event_block`
- `isc_que_events`
- `isc_event_counts`
- `isc_cancel_events`

It should not be built on `isc_wait_for_event`, because that would push the driver toward blocking behavior that is a poor fit for Xojo desktop apps and weaker than PostgreSQL's timer-driven polling model.

## Notify

`Notify(name)` is still viable, but it should be documented as a convenience wrapper that posts an event through PSQL, not as a literal clone of PostgreSQL's protocol command.

## Outcome

Phase 24 closes with a positive implementation recommendation:

- Firebird events are worth exposing
- the next phase should implement a limited event API
- the payload should be Firebird-native: `Name` plus `Count`
