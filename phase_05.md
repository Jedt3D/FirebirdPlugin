# Phase 05 Detailed Plan

## Status

Completed on April 6, 2026.

Completion result:

- desktop integration suite result: `100 passed, 0 failed`
- article: `phase_05_article.md`

Branch:

- `feature/phase-05`

## Objective

Phase 05 adds generated-key and `RETURNING` convenience around Xojo's native `DatabaseRow` insertion flow.

The goal is to make `Database.AddRow(...)` useful on Firebird without forcing users to hand-write `INSERT ... RETURNING` for common identity-primary-key workflows, while keeping custom multi-column `RETURNING` on the existing `SelectSQL` path.

## Version Target

Phase 05 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: `Database.AddRow` support

Implement the Xojo database-engine insert callback so this works:

- `db.AddRow("table", row)`

### Deliverable 2: generated-key return support

Implement the Xojo database-engine generated-key callback so this works:

- `Var newId As Integer = db.AddRow("table", row, "")`
- `Var newId As Integer = db.AddRow("table", row, "ID")`

### Deliverable 3: Firebird primary-key lookup for default ID column

When the caller passes an empty ID-column name, resolve the table primary key from Firebird metadata and use it in `RETURNING`.

### Deliverable 4: desktop integration coverage

Add tests that prove:

- `Database.AddRow` inserts a row successfully
- `Database.AddRow(..., "")` returns a generated key
- the inserted row can be read back by that key
- explicit ID-column selection also works

## In Scope

- Xojo `AddRow` engine callback support
- Xojo generated-key callback support
- Firebird metadata lookup for default primary key column
- typed binding for common `DatabaseRow` scalar values used in the test path
- README and example updates

## Out of Scope

- replacing the existing `SelectSQL("INSERT ... RETURNING ...")` path
- arbitrary multi-column generated-key objects
- `INT128` generated-key return values through the Xojo callback
- savepoints
- Services API
- Events API

## Design Rules

### Rule 1: keep custom `RETURNING` on `SelectSQL`

The plugin already supports Firebird `RETURNING` through `SelectSQL`.

Phase 05 only adds convenience for the common generated-key case.

### Rule 2: prefer native Xojo integration over new Firebird-only methods

If Xojo already provides a database-engine hook for row insertion and returned IDs, use that instead of inventing a parallel Firebird-only convenience API.

### Rule 3: keep generated-key return numeric

The Xojo callback returns `Integer`, so this phase targets common numeric primary-key workflows only.

## Success Criteria

Phase 05 is complete when:

- `Database.AddRow` works with the Firebird plugin
- `Database.AddRow(..., "")` returns a generated ID
- the desktop test suite passes with the new coverage included
- docs are updated
- the detailed phase article is written
