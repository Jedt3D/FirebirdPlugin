# Phase 02 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `78 passed, 0 failed`
- article: `phase_02_article.md`

Branch:

- `feature/phase-02`

## Objective

Phase 02 adds the first Firebird 4/5/6-specific type bridge that the Xojo plugin still lacks after Phase 01.

The goal is to support the modern Firebird types that matter to current engines without changing the overall plugin architecture:

- `INT128`
- `DECFLOAT(16)`
- `DECFLOAT(34)`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

## Version Target

Phase 02 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Phase 02 Design Choice

The Xojo boundary for these types will be text-first.

Phase 02 will not invent new custom Xojo value classes for:

- 128-bit integers
- decimal floating-point values
- time-zone-aware temporal structs

Instead:

- result-set reads will expose these values through `StringValue`
- prepared-statement input will accept Xojo `String` values
- the C++ wrapper will translate between string form and Firebird wire structs when the parameter metadata requires a modern type

This keeps the public API practical while still giving correct Firebird 4/5/6 behavior.

## Why This Approach

- Xojo has no native `Int128` type
- `Double` is not an acceptable public representation for `DECFLOAT`
- Xojo `DateTime` does not map cleanly to Firebird time-zone-aware wire structs without extra design work
- Jaybird and the .NET provider both preserve high-precision semantics instead of silently degrading them to `Double`
- the Firebird SDK exposes utility interfaces that can convert these modern structs safely without forcing a full rewrite to the interface-based execution API

## Deliverables

### Deliverable 1: Modern type read support

Add rowset support for:

- `SQL_INT128`
- `SQL_DEC16`
- `SQL_DEC34`
- `SQL_TIME_TZ`
- `SQL_TIME_TZ_EX`
- `SQL_TIMESTAMP_TZ`
- `SQL_TIMESTAMP_TZ_EX`

Expected Xojo behavior:

- `ColumnType` reports text for these types
- `Column(...).StringValue` returns a normalized textual representation

### Deliverable 2: Modern type string binding

Extend prepared-string binding so that when Firebird parameter metadata expects one of the modern types, the plugin converts the Xojo string into the proper Firebird binary value before execution.

This should work for:

- `PreparedStatement.Bind(index, value As String)`
- `Database.ExecuteSQL(sql, params())`
- `Database.SelectSQL(sql, params())`

### Deliverable 3: Firebird utility-interface bridge

Use Firebird utility interfaces only for conversion:

- `IInt128`
- `IDecFloat16`
- `IDecFloat34`
- `IUtil` time-zone encode/decode helpers

Do not migrate statement execution away from `isc_*` in this phase.

### Deliverable 4: Desktop integration coverage

Add tests for:

- `INT128` round-trip
- `DECFLOAT(16)` round-trip
- `DECFLOAT(34)` round-trip
- `TIME WITH TIME ZONE` round-trip
- `TIMESTAMP WITH TIME ZONE` round-trip

These tests must validate both:

- insert/bind behavior
- readback behavior

## In Scope

- utility-interface conversion helpers inside the existing C++ wrapper
- SQLDA buffer allocation for modern Firebird types
- result-set conversion of the modern Firebird types to strings
- type-aware string binding for the modern Firebird types
- desktop integration tests for all Phase 02 types
- example and README updates for the new usage pattern

## Out of Scope

- custom Xojo wrapper classes for `Int128`, decimal floating point, or zoned timestamps
- Services API
- Events API
- Array API
- transaction info helpers
- interface-based statement execution rewrite
- transaction parameter buffer redesign

## Public API Strategy

Phase 02 aims to avoid adding unnecessary new public methods.

Primary usage pattern:

- use `String` values for bind input
- use `StringValue` for readback

Examples:

- `ps.Bind(0, "123456789012345678901234567890")`
- `ps.Bind(1, "12345.6789")`
- `ps.Bind(2, "10:11:12.3456 UTC")`
- `ps.Bind(3, "2026-04-06 13:14:15.3456 UTC")`

## Internal Work Plan

### Step 1: Add conversion helpers

File targets:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Work:

- add helper functions for `INT128`, `DECFLOAT`, and time-zone-aware temporal encode/decode
- obtain Firebird utility interfaces through `fb_get_master_interface()`
- keep conversion code isolated from statement execution code

### Step 2: Extend SQLDA buffer allocation

Work:

- allocate correct buffer sizes for `SQL_INT128`, `SQL_DEC16`, `SQL_DEC34`
- allocate correct buffer sizes for time-zone-aware temporal structs

### Step 3: Extend output conversion

Work:

- decode modern result values into normalized strings
- map these types to Xojo text column types

### Step 4: Extend string binding

Work:

- detect when a string bind targets a modern Firebird type
- convert to the native binary struct instead of forcing `SQL_VARYING`

### Step 5: Add tests

Primary file target:

- `example-desktop/MainWindow.xojo_window`

Initial test names:

- `TestInt128RoundTrip`
- `TestDecFloatRoundTrip`
- `TestTimeWithTimeZoneRoundTrip`
- `TestTimestampWithTimeZoneRoundTrip`

## Success Criteria

Phase 02 is complete when:

- the plugin reads Firebird 4/5/6 modern types correctly
- string binding works for those same types
- desktop integration tests pass
- docs/examples show the string-based usage pattern clearly
