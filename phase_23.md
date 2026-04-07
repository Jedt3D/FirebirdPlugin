# Phase 23 Detailed Plan

Status: Complete on April 7, 2026  
Branch: `feature/phase-23`

## Goal

Evaluate whether the Firebird plugin should add a PostgreSQL-style large-object / streaming BLOB API, and determine the safest Xojo-visible shape if streaming BLOB support is worth exposing later.

## Research Question

The PostgreSQL parity target is:

- `CreateLargeObject() As Integer`
- `OpenLargeObject(oid As Integer, ReadOnly As Boolean = False) As PostgreSQLLargeObject`
- `DeleteLargeObject(oid As Integer)`
- `PostgreSQLLargeObject` with:
  - `Length`
  - `Position`
  - `Close`
  - `Read`
  - `Seek`
  - `Tell`
  - `Write`

Phase 23 asks whether Firebird can expose the same semantics honestly.

## Firebird Capability Findings

At the legacy C API level, Firebird already supports streaming-style blob operations:

- `isc_create_blob2`
- `isc_open_blob2`
- `isc_get_segment`
- `isc_put_segment`
- `isc_seek_blob`

That means:

- streaming read is possible
- streaming write is possible
- position-aware access is possible

However, there are constraints that matter for the Xojo public API:

1. Firebird BLOBs are locators, not PostgreSQL-style standalone large objects.
2. BLOB operations are transaction-bound.
3. Seek is only supported for stream BLOBs, not all BLOBs.
4. BLOB identifiers can be volatile before materialization into a table row.
5. Delete-by-id semantics are not a clean first-class match for PostgreSQL's `DeleteLargeObject`.

## Design Decision

Do not implement a PostgreSQL-style large-object surface directly at this stage.

Specifically, do not add these public methods yet:

- `CreateLargeObject`
- `OpenLargeObject`
- `DeleteLargeObject`
- `FirebirdLargeObject` as if it were equivalent to `PostgreSQLLargeObject`

Reason:

- the names would imply standalone object lifecycle semantics that Firebird does not cleanly share
- `DeleteLargeObject` is the weakest semantic fit
- a direct parity API would be easy to misunderstand and hard to document honestly

## What Would Fit Firebird Better

If streaming BLOB support is added later, it should be Firebird-native and explicit.

The cleaner direction is a dedicated handle type with transaction-bound semantics, for example:

- `OpenBlob(blobId As Int64, ReadOnly As Boolean = False) As FirebirdBlobHandle`
- `CreateBlob() As FirebirdBlobHandle`

And a handle object with methods similar to:

- `Close`
- `Read`
- `Write`
- `Seek`
- `Tell`
- `Length`
- `Position`
- `BlobId`

That design would still require one more major decision:

- how Xojo code obtains a stable BLOB identifier in a practical way

Without a good answer to that question, a streaming handle API would be technically possible but awkward in normal app code.

## Practical Recommendation

Phase 23 closes as a design / feasibility slice with a no-go decision on direct PostgreSQL parity.

Recommended next step:

- move to Firebird event / notification feasibility work first

Optional later follow-up:

- revisit a Firebird-native streaming BLOB handle only if:
  - real user demand exists
  - we define clean blob-id acquisition semantics
  - we document stream-only seek behavior clearly

## Source Basis

This phase was based on:

- local Xojo documentation for `PostgreSQLDatabase` and `PostgreSQLLargeObject`
- Firebird `ibase.h` and current plugin source
- official Firebird protocol / driver documentation showing:
  - open/create blob support
  - seek support for stream blobs
  - volatile versus materialized blob-id behavior
