# Phase 21 Detailed Plan

## Status

Complete on April 7, 2026.

Branch:

- `feature/phase-21`

## Objective

Phase 21 improves `RowSet` feel so the Firebird plugin behaves more like a first-class built-in Xojo driver during normal read-only navigation.

The goal is to close the most visible forward-only cursor limitation without taking on editable-row behavior yet.

## Deliverables

### Deliverable 1: buffered `RowSet` read navigation

Implement real support for:

- `RowCount`
- `MoveToPreviousRow`
- `MoveToFirstRow`
- `MoveToLastRow`

Behavior:

- work on normal `SelectSQL` result sets
- preserve existing forward iteration behavior
- support `BeforeFirstRow` / `AfterLastRow` transitions correctly

### Deliverable 2: desktop integration coverage

Add desktop tests that prove:

- ordered row navigation works in both directions
- `RowCount` is real instead of `-1`
- moving to BOF and then advancing again behaves correctly

## In Scope

- buffered cursor data inside the plugin cursor wrapper
- Xojo cursor callback wiring for previous/first/last navigation
- real `RowCount` based on buffered rows
- desktop integration tests
- documentation and example updates

## Out of Scope

- editable row/update/delete hooks
- asynchronous cursor behavior
- notification/event APIs
- large-object / streaming BLOB APIs
- broader SQL rewrite or statement-layer changes

## Design Rules

### Rule 1: improve read navigation without breaking current reads

Existing forward iteration and column access must keep working exactly as before.

### Rule 2: buffer at the cursor wrapper, not in SQL call sites

The `RowSet` improvement should remain an internal cursor concern so Xojo-facing API shape stays unchanged.

### Rule 3: do not pretend editability exists

Phase 21 is strictly a read-navigation parity slice. Updateable cursor hooks remain intentionally unimplemented.

## Success Criteria

Phase 21 is complete when:

- `RowSet` supports previous/first/last navigation
- `RowCount` returns a real value
- desktop tests prove BOF/EOF transitions and ordered reads
- docs and examples are updated
- the phase article is written

## Verification

Verified with the desktop suite on April 7, 2026:

- `172 passed`
- `0 failed`
