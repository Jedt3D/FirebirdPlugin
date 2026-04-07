# Phase 26 Article

Phase 23 answered a design question. Phase 26 turns that answer into a real API.

The plugin now ships a Firebird-native blob foundation:

- `CreateBlob`
- `OpenBlob`
- `BindBlob`
- `FirebirdBlob`

## Why This Matters

There was a real parity gap here.

PostgreSQL has large objects. SQLite has blob objects. Firebird already had direct text and binary blob helpers, but it did not yet have a clean streaming object for partial reads and writes.

The wrong answer would have been to fake PostgreSQL names. Firebird blob locators are transaction-bound, and the lifecycle does not map cleanly to `CreateLargeObject` / `OpenLargeObject` / `DeleteLargeObject`.

So Phase 26 ships the right thing instead:

- a Firebird-native blob object with honest semantics

## What the Plugin Does

The new `FirebirdBlob` object supports:

- `Read`
- `Write`
- `Seek`
- `Close`
- `BlobId`
- `Length`
- `Position`
- `IsOpen`

Creation and reading are intentionally separated:

- `CreateBlob()` for write flow
- `OpenBlob(rowset, column)` for read flow

That keeps the Xojo surface tied to real Firebird behavior instead of inventing a global blob-id lifecycle.

## The Important Technical Choice

The most delicate part was `Seek`.

Firebird's lower-level segmented blob behavior is not a clean match for the simple absolute seek contract Xojo code expects. So the plugin implements deterministic seek behavior for read blobs by reopening and skipping to the requested offset.

That is the correct tradeoff:

- predictable for Xojo developers
- still honest about Firebird's underlying blob model

## Result

Phase 26 materially improves the plugin's built-in-driver feel in a place real applications notice.

The Firebird plugin now has a real streaming BLOB workflow, while still preserving Firebird's actual engine semantics instead of pretending to be PostgreSQL.
