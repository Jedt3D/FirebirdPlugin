# Phase 05 Article

## Summary

Phase 05 added native Xojo generated-key convenience on top of Firebird `RETURNING`.

This phase did not replace the existing Firebird-specific `SelectSQL("INSERT ... RETURNING ...")` path. Instead, it integrated the plugin with Xojo's own `Database.AddRow(...)` callbacks so common identity-primary-key workflows now feel native in Xojo code.

Final verification result on April 6, 2026:

- desktop integration suite: `100 passed, 0 failed`

Branch:

- `feature/phase-05`

## What Was Added

### 1. Xojo `Database.AddRow` engine integration

File:

- `sources/FirebirdPlugin.cpp`

Added engine callbacks:

- `fbEngineAddTableRecord`
- `fbEngineAddRowWithReturnValue`

These are now wired into the Xojo database-engine definition, which means the plugin supports:

- `db.AddRow("table", row)`
- `db.AddRow("table", row, "")`
- `db.AddRow("table", row, "ID")`

### 2. Firebird metadata lookup for default generated-ID column

Files:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Added:

- `primaryKeyColumnSQL()`

This query resolves the table primary key from Firebird system tables when Xojo calls:

- `db.AddRow("table", row, "")`

That lets the plugin build:

- `INSERT ... RETURNING <primary_key_column>`

without requiring the caller to specify the column name manually.

### 3. Insert builder and typed binding for `DatabaseRow`

File:

- `sources/FirebirdPlugin.cpp`

Added internal helpers to:

- collect `REALcolumnValue` entries from Xojo
- build an `INSERT` statement with placeholders
- bind common scalar values through the existing statement layer
- execute `RETURNING` for generated-ID flows
- extract the returned numeric key back into Xojo's `Integer`

Binding coverage in this phase is intentionally targeted at common `DatabaseRow` usage:

- strings
- integer-like values
- floating and decimal values
- booleans
- nulls
- binary payload fallback through blob binding when the column type is binary

### 4. Desktop integration coverage

File:

- `example-desktop/MainWindow.xojo_window`

Added tests:

- `TestDatabaseAddRow`
- `TestDatabaseAddRowWithReturnValue`

The tests prove:

- `Database.AddRow` inserts a row successfully
- `Database.AddRow(..., "")` returns a generated identity value
- the inserted row can be fetched back using that returned key
- explicit generated-ID column selection also works

### 5. Public docs and examples

Files:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_05.md`

Changes:

- README now documents native `Database.AddRow` generated-ID support
- quick reference includes the `DatabaseRow` insert pattern
- API examples now show native generated-ID insertion
- the master plan now marks generated-key convenience complete in Phase 05

## What Phase 05 Deliberately Did Not Do

This phase did not add:

- a new Firebird-only generated-key helper method
- multi-column `RETURNING` convenience objects
- generic arbitrary result objects for insert-return flows
- `INT128` generated keys through the Xojo callback
- Services API
- Events API

That boundary was intentional. Xojo already has a native insert-and-return-ID path, so Phase 05 used that integration point instead of inventing a parallel API.

## Important Runtime Note

During the same test suite, the Phase 04 read-only transaction test still raises:

- error `-817`
- `attempted update during read-only transaction`

That is expected behavior and remains a passing condition. It proves Firebird is enforcing the read-only transaction correctly.

## Outcome

Phase 05 closes the common generated-key gap cleanly.

The plugin now supports two levels of insert-return behavior:

- native Xojo generated-key convenience through `Database.AddRow(...)`
- full Firebird-specific custom `RETURNING` through `SelectSQL("INSERT ... RETURNING ...")`

The next logical step is no longer generated keys. The best next target is:

- Services API scope for backup, restore, statistics, and administrative workflows

or, if transaction scope should continue first:

- decide whether savepoints and richer multi-column `RETURNING` helpers belong in the public Xojo surface
