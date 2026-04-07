# Phase 26 Detailed Plan

Status: Complete on April 7, 2026  
Branch: `feature/phase-26`

## Goal

Ship a real Firebird-native blob foundation slice that gives Xojo code streaming read/write behavior without pretending Firebird blobs are PostgreSQL large objects.

Phase 23 already answered the parity question:

- do not mimic `CreateLargeObject` / `OpenLargeObject` / `DeleteLargeObject`
- do ship a practical Firebird-native object if the semantics can stay honest

Phase 26 is that shipped implementation.

## Public Xojo Surface

Phase 26 adds:

- `FirebirdDatabase.CreateBlob() As FirebirdBlob`
- `FirebirdDatabase.OpenBlob(rowset As RowSet, column As String) As FirebirdBlob`
- `FirebirdPreparedStatement.BindBlob(index As Integer, value As FirebirdBlob)`

And the new `FirebirdBlob` class:

- `BlobId As String`
- `Length As Int64`
- `Position As Int64`
- `IsOpen As Boolean`
- `Read(count As Integer) As MemoryBlock`
- `Write(value As MemoryBlock) As Boolean`
- `Seek(offset As Int64, whence As Integer) As Int64`
- `Close() As Boolean`

## Why This Shape Is Correct

Firebird blob locators are transaction-bound. That means a PostgreSQL-style lifecycle surface would be misleading in two ways:

1. blob handles are not global independent objects in the PostgreSQL sense
2. a fake `DeleteLargeObject()` style method would suggest semantics the engine does not actually provide cleanly

So the public Xojo API stays Firebird-native:

- create a writable blob
- bind it into a prepared statement
- reopen it from a `RowSet` column when reading
- stream with explicit `Read`, `Write`, and `Seek`

## Implementation Strategy

The native layer adds a dedicated `FBBlob` wrapper over Firebird blob handles.

Write path:

1. `CreateBlob()` opens a writable blob with `isc_create_blob2`
2. `Write()` sends segments through `isc_put_segment`
3. `BindBlob()` binds the resulting blob id into a prepared statement

Read path:

1. `OpenBlob()` resolves the `RowSet` column value to a blob id
2. `Read()` pulls data through `isc_get_segment`
3. `Length` is exposed as typed blob metadata

Important design choice:

- `Seek()` is exposed with a simple Xojo-facing contract
- for read blobs it uses deterministic reopen-and-skip behavior instead of relying on segmented-blob seek semantics that do not map cleanly

That gives predictable behavior without inventing PostgreSQL large-object semantics.

## Scope Boundaries

Phase 26 intentionally does not add:

- PostgreSQL-style large-object lifecycle names
- a fake blob-id API detached from `RowSet` / transaction context
- service-manager or engine-wide blob deletion semantics

## Test Scope

Desktop verification covers:

1. `CreateBlob`
2. chunked `Write`
3. `BlobId`
4. `BindBlob`
5. `OpenBlob`
6. `Length`
7. partial `Read`
8. deterministic `Seek`
9. full readback after seek
10. explicit `Close`

The coverage lives in:

- `TestBlobStreaming`

## Result

Phase 26 closes successfully as a shipped implementation slice.

Verified suite result:

- `190 passed`
- `0 failed`

## Practical Outcome

The Firebird plugin now closes the biggest remaining BLOB ergonomics gap without lying about engine semantics.

It still does not pretend to be PostgreSQL's large-object system, but it now gives Xojo developers a real, testable streaming BLOB workflow that fits Firebird correctly.
