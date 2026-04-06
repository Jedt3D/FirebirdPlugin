# Phase 18 Detailed Plan

## Status

Complete on April 7, 2026.

Branch:

- `feature/phase-18`

## Objective

Phase 18 starts the built-in-parity enhancement track by adding affected-row reporting to `FirebirdDatabase`.

The goal is to expose a simple Xojo-facing `AffectedRowCount` property that behaves like a first-class built-in-driver feature without disturbing the existing `SelectSQL` / `RowSet` workflow.

## Version Target

Phase 18 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: `AffectedRowCount` on `FirebirdDatabase`

Implement:

- `AffectedRowCount As Int64`

Behavior:

- update after direct `ExecuteSQL`
- update after prepared `ExecuteSQL`
- update after native `AddRow`
- preserve the last non-query count across `SelectSQL`
- reset on connect/disconnect

### Deliverable 2: desktop integration coverage

Add desktop tests that prove:

- `INSERT` sets `AffectedRowCount` to `1`
- `UPDATE` sets `AffectedRowCount` to `1`
- `DELETE` sets `AffectedRowCount` to `1`
- prepared `ExecuteSQL` sets `AffectedRowCount` correctly
- `SelectSQL` does not erase the previous non-query count

## In Scope

- `AffectedRowCount` property exposure in the plugin class table
- native tracking through non-query execution paths
- desktop integration tests
- documentation and example updates

## Out of Scope

- statement-by-statement row counts for `SELECT`
- richer cursor navigation
- row-edit/update hooks on `RowSet`
- SSL/TLS connection controls
- event APIs
- large-object / streaming BLOB object work

## Design Rules

### Rule 1: only non-query execution paths update the count

`AffectedRowCount` must reflect the last meaningful DML-style execution result, not arbitrary cursor fetch operations.

### Rule 2: `SelectSQL` must not clear the previous DML count

Application code often issues a follow-up read after an `INSERT`, `UPDATE`, or `DELETE`. The property should remain useful across that read.

### Rule 3: preserve existing behavior while adding parity

Phase 18 must not regress:

- modern Firebird type handling
- BLOB round-trip behavior
- service-manager operations
- native `AddRow`

## Success Criteria

Phase 18 is complete when:

- `AffectedRowCount` is exposed on `FirebirdDatabase`
- direct `ExecuteSQL`, prepared `ExecuteSQL`, and `AddRow` update it correctly
- `SelectSQL` preserves the previous non-query count
- the desktop suite passes with the new coverage included
- docs and examples are updated
- the phase article is written

## Verification

Verified with the desktop suite on April 7, 2026:

- `162 passed`
- `0 failed`
