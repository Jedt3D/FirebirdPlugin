# Phase 01 Article

## Summary

Phase 01 completed the first real feature pass of the Firebird Xojo plugin for Firebird 4, 5, and 6.

The goal of this phase was to extend the existing legacy `isc_*` implementation without changing the plugin architecture. The work stayed within the current `FirebirdDB` low-level wrapper and `FirebirdPlugin` Xojo bridge, and focused on practical user-facing features that map cleanly to Xojo's `Database`, `PreparedStatement`, and `RowSet` APIs.

Phase 01 was implemented on branch `feature/phase-01`.

## What Was Added

### 1. Database info support

The plugin now exposes a curated typed subset of `isc_database_info` through `FirebirdDatabase`:

- `ServerVersion() As String`
- `PageSize() As Integer`
- `DatabaseSQLDialect() As Integer`
- `ODSVersion() As String`
- `IsReadOnly() As Boolean`

This work was added at two levels:

- low-level parsing helpers in `FBDatabase`
- Xojo-facing methods in `FirebirdDatabase`

This gives the plugin its first explicit Firebird-specific metadata surface beyond SQL and schema queries.

### 2. Prepared-statement temporal binds

The plugin now supports binding a Xojo `DateTime` directly to Firebird temporal parameters through:

- `Bind(index As Integer, value As DateTime)`

The bridge selects the correct Firebird bind behavior from the prepared parameter metadata:

- Firebird `DATE` -> date portion only
- Firebird `TIME` -> time portion only
- Firebird `TIMESTAMP` -> full timestamp

This was implemented using the existing low-level `FBStatement::bindDate`, `bindTime`, and `bindTimestamp` support that already existed internally.

### 3. Prepared-statement BLOB binds

The plugin now exposes explicit BLOB bind methods:

- `BindTextBlob(index As Integer, value As String)`
- `BindBinaryBlob(index As Integer, value As MemoryBlock)`

This keeps text and binary BLOB handling explicit and avoids overload ambiguity on the Xojo side.

### 4. Variant param-array binding improvements

The generic `SelectSQL(..., params...)` and `ExecuteSQL(..., params...)` parameter path was extended so it now understands:

- `DateTime`
- `MemoryBlock`

That means Firebird temporal and BLOB parameter handling works not only through direct prepared statement methods, but also through the general Variant-based SQL execution path where appropriate.

### 5. Firebird-specific SQL validation

Phase 01 added coverage for Firebird-specific SQL behavior:

- `RETURNING`
- `EXECUTE BLOCK`
- `EXECUTE PROCEDURE`

This matters because these are not just syntax features. They validate statement preparation, execution, row-return behavior, and the plugin's mapping of Firebird-specific statement patterns into Xojo `RowSet` behavior.

### 6. Native `BOOLEAN` and scaled numeric validation

The test suite now explicitly validates:

- native Firebird `BOOLEAN`
- scaled `NUMERIC` / `DECIMAL`

This closes two important gaps:

- boolean behavior is no longer only indirectly covered through numeric compatibility patterns
- scaled numerics are now verified as actual driver behavior, not assumed behavior

## Internal Fixes Made During Phase 01

### 1. `EXECUTE PROCEDURE` singleton row handling

Firebird executable procedures return output differently from a normal `SELECT`.

The low-level statement layer already used `isc_dsql_execute2` for executable procedures, but the cursor path still assumed fetch-driven row retrieval. That was corrected so procedure output rows are surfaced properly through Xojo `RowSet`.

### 2. Date/time row payload compatibility

Temporal round-trip tests initially failed even though Firebird encode/decode itself was correct.

The real issue was the shape of the `dbDate`, `dbTime`, and `dbTimeStamp` payloads being handed to Xojo. The plugin was updated to match the byte-order convention used by Xojo's own database plugins, which fixed:

- `DATE` readback
- `TIME` readback
- `TIMESTAMP` readback

### 3. Scaled numeric rounding

Scaled numerics initially truncated during bind conversion, which produced a readback mismatch for decimal values like `1234.56`.

The bind path now rounds instead of truncating when converting scaled floating-point values into Firebird integer storage formats.

### 4. Plugin load-path issue in Xojo

During validation, the Xojo IDE kept reporting that newly added plugin methods did not exist.

The root cause was not the source code. It was a stale duplicate `FirebirdPlugin.xojo_plugin` in the Xojo app root, while the new plugin had been installed correctly in the `Plugins/` folder.

Once the stale duplicate was removed from Xojo's load path, the IDE correctly picked up the new plugin methods.

## Tests Added in Phase 01

The desktop integration suite was expanded with the following tests:

- `TestDatabaseInfo`
- `TestPreparedStatementBindTemporalTypes`
- `TestPreparedStatementBindBlobs`
- `TestNativeBooleanRoundTrip`
- `TestScaledNumericRoundTrip`
- `TestReturningClause`
- `TestExecuteBlock`
- `TestExecuteProcedure`
- `TestEmbeddedConnect`

## Final Test Result

Phase 01 finished with the full desktop test suite passing.

Validation result on April 6, 2026:

- Passed: 68
- Failed: 0
- Result: `ALL TESTS PASSED`

Notable environment detail:

- hostless local attachment did not work with the current Firebird client/runtime setup on this machine
- the test was changed to treat that as environment capability information instead of a driver defect

## Files Changed

Core implementation:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Tests:

- `example-desktop/MainWindow.xojo_window`

Planning and documentation:

- `firebird_xojo_detail_plan.md`
- `phase_01.md`
- `README.md`
- `examples/FirebirdExample_API2.xojo_code`
- `examples/QuickReference.xojo_code`

Phase completion record:

- `phase_01_article.md`

## Outcome

Phase 01 achieved the intended goal: extend the plugin in meaningful Firebird-specific ways while keeping the current legacy API architecture intact and shippable.

At the end of Phase 01, the plugin now has:

- Firebird database metadata helpers
- practical temporal binding
- explicit text and binary BLOB binding
- validated Firebird-specific SQL behavior
- improved boolean and scaled numeric coverage
- a materially stronger integration test suite

This closes the most important first-wave gaps without forcing an interface API rewrite or a larger type-system migration.
