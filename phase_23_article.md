# Phase 23 Article

Phase 23 did not add a new public API. It closes the large-object parity question by deciding what not to ship.

## The Goal

The parity target was Xojo's PostgreSQL large-object workflow:

- `CreateLargeObject`
- `OpenLargeObject`
- `DeleteLargeObject`
- a dedicated object with `Read`, `Write`, `Seek`, `Tell`, `Length`, and `Position`

Because PostgreSQL is the main parity reference for this plugin, this was the right next question to ask.

## What the Research Found

Firebird can stream BLOBs at the API level. The legacy API already supports:

- create/open
- segmented read/write
- seek

So the problem is not raw capability.

The problem is semantics.

PostgreSQL large objects are standalone database objects identified by an integer OID. Firebird BLOBs are locators bound to transactions and row materialization behavior. That makes the apparent API match much weaker than it looks at first.

Two issues matter most:

1. `DeleteLargeObject` is not a clean semantic fit.
2. A public streaming API still needs a practical way for Xojo code to obtain stable BLOB identifiers.

There is also an important technical limitation:

- seek is only supported for stream BLOBs

That means a PostgreSQL-like object would either over-promise or require caveats strong enough to make the direct parity naming misleading.

## Decision

Do not add PostgreSQL-style large-object APIs yet.

That means:

- no `CreateLargeObject`
- no `OpenLargeObject`
- no `DeleteLargeObject`
- no fake `FirebirdLargeObject` parity class

## Better Direction

If streaming support becomes worth exposing later, the better design is Firebird-native:

- a dedicated handle object
- explicit transaction-bound behavior
- explicit documentation that seek is only safe on stream BLOBs
- explicit blob-id handling instead of pretending the objects are standalone

## Outcome

Phase 23 is complete as a design slice.

It reduces risk by preventing a misleading public API and makes the next priorities clearer:

1. event / notification feasibility
2. Firebird-native streaming BLOB handle only if the blob-id ergonomics are solved cleanly
