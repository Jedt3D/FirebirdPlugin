# Phase 01 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `68 passed, 0 failed`
- article: `phase_01_article.md`

## Objective

Phase 01 is the first implementation pass for the Firebird Xojo plugin after defining the overall roadmap.

The goal of this phase is to close the highest-value gaps in the current legacy-API implementation without changing the API family.

This phase is intentionally limited to features that are:

- directly useful to Xojo users
- compatible with the current `isc_*` implementation strategy
- strongly supported by Jaybird as the primary behavioral model
- practical to validate with local integration tests

## Version Target

Phase 01 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

Phase 01 does not attempt to preserve compatibility behavior for Firebird 2.5 or 3.x.

## Phase 01 Deliverables

### Deliverable 1: Database Info support

Add Firebird database-info support to the plugin using `isc_database_info`.

This is the first explicitly Firebird-specific metadata feature that should be exposed beyond SQL and schema queries.

#### Phase 01 database info scope

Expose a curated typed subset first, not a raw low-level buffer API.

Initial target items:

- engine/server version
- page size
- database SQL dialect
- ODS version
- read-only flag

Optional in Phase 01 if implementation cost stays low:

- attachment count
- implementation/platform string

#### Why this is in Phase 01

- it is directly supported by the Firebird C SDK
- it is useful for diagnostics and support
- it is low risk compared to arrays, events, services, or request APIs
- Jaybird and the .NET Provider both treat database metadata as normal provider surface, not as a special advanced feature

### Deliverable 2: Expose missing prepared-statement bind types

The C++ layer already has internal support for:

- BLOB bind
- DATE bind
- TIME bind
- TIMESTAMP bind

Phase 01 exposes these to Xojo.

### Deliverable 3: Expand the local integration test suite

Add tests that prove:

- database info retrieval works
- BLOB round-trips work
- temporal bind round-trips work
- Firebird-specific SQL behavior works
- embedded/local attachment works

### Deliverable 4: Update examples and docs

After implementation and tests are stable:

- update `README.md`
- update `examples/FirebirdExample_API2.xojo_code`
- update `examples/QuickReference.xojo_code` if needed

## In Scope

- `isc_database_info` wrapper and Xojo-visible API
- prepared bind support for date, time, timestamp, and BLOB
- tests for text and binary BLOB handling
- tests for Firebird-specific SQL:
  - `RETURNING`
  - `EXECUTE BLOCK`
  - executable stored procedures
- embedded/local attach test
- native `BOOLEAN` and scaled numeric validation where needed to support the new tests

## Out of Scope

- Services API
- Event API
- Array API
- Transaction info API
- BLR/request-style APIs
- interface-based API migration
- full Firebird 4/5/6 new-type implementation:
  - `INT128`
  - `DECFLOAT`
  - `TIME WITH TIME ZONE`
  - `TIMESTAMP WITH TIME ZONE`

These stay out of Phase 01 so the first pass remains focused and shippable.

## Design Rules for Phase 01

### Rule 1: Keep the current architecture

Do not rewrite the plugin around the newer interface API in this phase.

Keep:

- `FirebirdDB` as the low-level wrapper
- `FirebirdPlugin` as the Xojo bridge
- SQL and prepared statements as the center of the public API

### Rule 2: Prefer typed Xojo-facing methods over raw Firebird buffers

The plugin should not expose Firebird info-item buffers directly to Xojo.

Instead:

- use low-level generic info retrieval internally
- expose only curated typed methods to Xojo users

### Rule 3: Every new feature must get an integration test

No new Xojo-visible feature should be added without a matching local desktop test in `example-desktop/MainWindow.xojo_window`.

## Proposed Public API Additions

These are the proposed Phase 01 Xojo-facing additions.

Exact signatures can be adjusted during implementation if the SDK imposes constraints, but the functional surface should remain equivalent.

## FirebirdDatabase additions

### Database info methods

Target methods:

- `ServerVersion() As String`
- `PageSize() As Integer`
- `DatabaseSQLDialect() As Integer`
- `ODSVersion() As String`
- `IsReadOnly() As Boolean`

Optional if cheap:

- `AttachmentCount() As Integer`
- `Implementation() As String`

### Notes

- keep these Firebird-specific methods on `FirebirdDatabase`
- do not try to force them into generic `Database`
- avoid a generic `DatabaseInfo(code As Integer) As String` unless the typed approach becomes impossible

## FirebirdPreparedStatement additions

### Temporal bind methods

Target methods:

- `Bind(index As Integer, value As DateTime)` for DATE/TIME/TIMESTAMP parameters

Implementation rule:

- inspect parameter metadata first
- if Firebird expects `DATE`, bind only the date portion
- if Firebird expects `TIME`, bind only the time portion
- if Firebird expects `TIMESTAMP`, bind full timestamp

If SDK constraints make `DateTime` binding awkward, fallback option:

- `BindDate(index As Integer, value As DateTime)`
- `BindTime(index As Integer, value As DateTime)`
- `BindTimestamp(index As Integer, value As DateTime)`

### BLOB bind methods

Target methods:

- `BindTextBlob(index As Integer, value As String)`
- `BindBinaryBlob(index As Integer, value As MemoryBlock)`

Reason for separate methods:

- avoids overload ambiguity
- keeps text vs binary explicit
- maps well to the existing internal implementation

## Internal C++ Work Plan

## Step 1: Add low-level database-info support

### File targets

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

### Required additions

Add a generic low-level helper to request database-info items.

Suggested shape:

- internal method to submit one or more info items
- parser helpers for:
  - VAX integers
  - counted strings
  - simple item scanning

Suggested internal methods:

- `bool databaseInfo(const std::vector<unsigned char>& items, std::vector<unsigned char>& out)`
- typed helpers built on top of that for the small curated subset

### Firebird info items to use first

- `isc_info_firebird_version`
- `isc_info_page_size`
- `isc_info_db_sql_dialect`
- `isc_info_ods_version`
- `isc_info_ods_minor_version`
- `isc_info_db_read_only`

Optional:

- `isc_info_attachment_id`
- `isc_info_active_transactions`
- implementation / base-level info items if useful and stable

## Step 2: Expose database info to Xojo

### File targets

- `sources/FirebirdPlugin.cpp`
- `sources/FirebirdPlugin.h` only if additional class state is needed

### Required additions

Add new `REALmethodDefinition` entries on `FirebirdDatabase`.

Add method callbacks that:

- validate the instance and connection state
- call the new `FBDatabase` info helpers
- return Xojo-friendly scalar results

### Error behavior

- on disconnected database, follow current plugin error style
- do not silently invent values
- surface failure through existing error code/message behavior where possible

## Step 3: Expose missing bind types

### File targets

- `sources/FirebirdPlugin.cpp`

### Required additions

Add Xojo-visible bind methods that bridge to:

- `FBStatement::bindDate`
- `FBStatement::bindTime`
- `FBStatement::bindTimestamp`
- `FBStatement::bindBlob`

### Implementation notes

- use parameter metadata from `describe_bind` to select the correct temporal mapping
- for binary BLOBs, prefer `MemoryBlock`
- for text BLOBs, send UTF-8 bytes from `String`

## Step 4: Validate SQL paths already present internally

### File targets

- `example-desktop/MainWindow.xojo_window`

### New tests to add

- `TestDatabaseInfo`
- `TestPreparedStatementBindDate`
- `TestPreparedStatementBindTime`
- `TestPreparedStatementBindTimestamp`
- `TestPreparedStatementBindTextBlob`
- `TestPreparedStatementBindBinaryBlob`
- `TestReturningClause`
- `TestExecuteBlock`
- `TestExecuteProcedure`
- `TestEmbeddedConnect`

Optional if time permits:

- `TestNativeBooleanRoundTrip`
- `TestScaledNumericRoundTrip`

## Step 5: Update docs

### File targets

- `README.md`
- `examples/FirebirdExample_API2.xojo_code`
- `examples/QuickReference.xojo_code`

### Required updates

- add database-info examples
- add temporal bind examples
- add text and binary BLOB bind examples
- add Phase 01-supported Firebird-specific SQL examples if missing

## Detailed Test Plan

## 1. Database info tests

### `TestDatabaseInfo`

Checks:

- `ServerVersion()` returns non-empty text
- `PageSize()` returns a positive integer
- `DatabaseSQLDialect()` returns expected dialect
- `ODSVersion()` returns non-empty text
- `IsReadOnly()` returns `False` for the local writable test DB

Firebird SDK area:

- informational functions

Representative API:

- `isc_database_info`

## 2. Temporal bind tests

### `TestPreparedStatementBindDate`

Checks:

- create or recreate a small test table with a DATE column
- bind a `DateTime`
- verify stored date matches expected date

Firebird SDK area:

- statement execution
- type conversions

### `TestPreparedStatementBindTime`

Checks:

- create or recreate a small test table with a TIME column
- bind a `DateTime`
- verify stored time matches expected time

### `TestPreparedStatementBindTimestamp`

Checks:

- create or recreate a small test table with a TIMESTAMP column
- bind a `DateTime`
- verify stored timestamp matches expected value

## 3. BLOB tests

### `TestPreparedStatementBindTextBlob`

Checks:

- create or recreate a table with `BLOB SUB_TYPE TEXT`
- bind text through the new text-BLOB API
- read it back through `SelectSQL`
- verify UTF-8 round-trip

### `TestPreparedStatementBindBinaryBlob`

Checks:

- create or recreate a table with binary BLOB
- bind a `MemoryBlock`
- read it back
- compare byte-for-byte

Firebird SDK area:

- blob functions

Representative APIs:

- `isc_create_blob2`
- `isc_put_segment`
- `isc_open_blob2`
- `isc_get_segment`

## 4. Firebird-specific SQL tests

### `TestReturningClause`

Checks:

- insert with `RETURNING`
- verify the returned value is correct and usable

### `TestExecuteBlock`

Checks:

- run a simple `EXECUTE BLOCK`
- verify side effects or returned values

### `TestExecuteProcedure`

Checks:

- create a simple stored procedure returning output values
- call it through SQL
- verify output row handling

Firebird SDK area:

- statement execution
- statement type inspection

Representative APIs:

- `isc_dsql_prepare`
- `isc_dsql_execute`
- `isc_dsql_execute2`
- `isc_dsql_sql_info`

## 5. Embedded/local attach test

### `TestEmbeddedConnect`

Checks:

- connect without host name using a local database path
- verify basic query succeeds

Notes:

- this test depends on how the local Firebird client/runtime is installed
- if environment setup makes this too brittle for every run, keep it separately runnable

## Execution Order

Recommended implementation order:

1. add low-level database-info support in `FirebirdDB`
2. expose database-info methods on `FirebirdDatabase`
3. add `TestDatabaseInfo`
4. expose temporal bind methods
5. add temporal tests
6. expose text and binary BLOB bind methods
7. add BLOB tests
8. add `RETURNING`, `EXECUTE BLOCK`, and stored-procedure tests
9. add embedded/local attach test
10. update docs and examples

Reason for this order:

- database info is self-contained and requested explicitly
- temporal and BLOB binds already exist internally, so exposure is a natural next step
- SQL-path tests build on the same prepared/execution infrastructure
- embedded testing is environment-sensitive, so it should come after the core work is stable

## Acceptance Criteria

Phase 01 is complete when all of the following are true:

- `FirebirdDatabase` exposes the curated database-info methods
- `FirebirdPreparedStatement` exposes temporal and BLOB bind support
- the local desktop test suite contains the new Phase 01 tests
- the new tests pass against the target environment
- README and examples reflect the newly supported APIs
- no regression is introduced in the existing SQL, transaction, and schema tests

## Risks and Constraints

### Xojo SDK binding complexity

The biggest implementation risk is not Firebird itself. It is mapping Xojo runtime types cleanly through the plugin SDK.

Specific risk points:

- `DateTime` extraction in plugin callbacks
- `MemoryBlock` handling for binary BLOBs
- overload selection for bind methods

### Embedded/local test variability

Embedded/local attach behavior depends on how Firebird client/runtime files are installed on the machine running tests. That makes this test more fragile than pure remote-attach tests.

### Firebird 4/5/6 types are intentionally deferred

This phase does not implement:

- `INT128`
- `DECFLOAT`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

That is a conscious choice to ship the first high-value extension pass without mixing it with a larger type-system expansion.

## References to Use During Phase 01

Primary:

- `firebird_xojo_detail_plan.md`
- Jaybird manual and repository

Secondary:

- Firebird .NET Provider docs and tests
- Python `firebird-driver` tests
- `firebird-qa` scenarios for SQL edge cases

## Repository Files Expected to Change in Phase 01

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`
- `README.md`
- `examples/FirebirdExample_API2.xojo_code`
- `examples/QuickReference.xojo_code`
