# Phase 02 Article

## Summary

Phase 02 completed the first Firebird 4/5/6-specific type pass for the Xojo plugin.

This phase kept the existing architecture intact:

- statement execution still uses the legacy `isc_*` API
- the plugin still presents a normal Xojo `Database` / `PreparedStatement` / `RowSet` surface
- Firebird utility interfaces are used only for conversion of modern Firebird types

## What Was Added

### 1. Firebird 4/5/6 modern type support

The plugin now supports:

- `INT128`
- `DECFLOAT(16)`
- `DECFLOAT(34)`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

### 2. String-first Xojo boundary for modern types

These values are exposed through:

- `PreparedStatement.Bind(index, value As String)`
- `Database.ExecuteSQL(sql, params())`
- `Database.SelectSQL(sql, params())`
- `RowSet.Column(...).StringValue`

This avoids lossy or awkward mappings for:

- 128-bit integers
- decimal floating point
- time-zone-aware temporal values

### 3. Hybrid conversion model

Phase 02 added a small conversion bridge to Firebird utility interfaces:

- `IInt128`
- `IDecFloat16`
- `IDecFloat34`
- `IUtil` time-zone encode/decode helpers

This bridge is used only for type conversion. The plugin was not migrated to interface-based statement execution in this phase.

## Internal Implementation Details

### `sources/FirebirdDB.cpp`

Added:

- Firebird utility-interface access via `fb_get_master_interface()`
- conversion helpers for:
  - `FB_I128`
  - `FB_DEC16`
  - `FB_DEC34`
  - `ISC_TIME_TZ`
  - `ISC_TIMESTAMP_TZ`
- SQLDA buffer allocation for:
  - `SQL_INT128`
  - `SQL_DEC16`
  - `SQL_DEC34`
  - `SQL_TIME_TZ`
  - `SQL_TIME_TZ_EX`
  - `SQL_TIMESTAMP_TZ`
  - `SQL_TIMESTAMP_TZ_EX`
- result-set decoding of those types into normalized text
- type-aware string binding for the same modern types
- `INT128` support for both:
  - plain `INT128`
  - scaled `NUMERIC(38, n)` / `DECIMAL(38, n)` values that arrive through `SQL_INT128`

### `sources/FirebirdPlugin.cpp`

Updated:

- Firebird type mapping so the modern Firebird types report as text to Xojo
- rowset value conversion so these values are surfaced as `StringValue` instead of falling through to raw bytes

## Tests Added in Phase 02

Added desktop integration tests in `example-desktop/MainWindow.xojo_window`:

- `TestInt128RoundTrip`
- `TestDecFloatRoundTrip`
- `TestTimeWithTimeZoneRoundTrip`
- `TestTimestampWithTimeZoneRoundTrip`

These tests verify:

- prepared-string binding
- `ExecuteSQL` param-array string binding
- result-set readback
- parity with Firebird’s own string cast output

## Documentation Updates

Updated:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_02.md`

The docs now describe the Phase 02 rule explicitly:

- modern Firebird types are bound with `String`
- modern Firebird types are read through `StringValue`

## Final Test Result

User-run desktop test suite result on April 6, 2026:

- `78 passed, 0 failed`

This includes the new Phase 02 coverage for:

- `INT128`
- `NUMERIC(38,4)`
- `DECFLOAT(16)`
- `DECFLOAT(34)`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

## Files Changed

- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`
- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_02.md`
- `phase_02_article.md`

## Outcome

Phase 02 closes the main Firebird 4/5/6 type gap without destabilizing the plugin architecture.

The next logical phase is broader Firebird-specific capability work:

- transaction info
- richer transaction controls
- Services API
