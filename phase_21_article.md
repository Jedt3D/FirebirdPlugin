# Phase 21 Article

## Summary

Phase 21 closes the most visible `RowSet` ergonomics gap by adding buffered read-navigation support to Firebird query results.

The plugin now supports:

- `RowCount`
- `MoveToPreviousRow`
- `MoveToFirstRow`
- `MoveToLastRow`

This makes ordinary Xojo `RowSet` usage feel much closer to a built-in database driver while keeping the Firebird implementation honest and read-only.

## What Was Added

### 1. Buffered cursor rows inside the plugin

The cursor wrapper now caches fetched rows so the plugin can move backward and answer `RowCount` without asking Firebird for a native scrollable cursor.

### 2. Real Xojo navigation callbacks

The Xojo cursor definition now implements:

- previous-row navigation
- first-row navigation
- last-row navigation

These callbacks were previously absent.

### 3. Real `RowCount`

`RowCount` no longer reports an unknown placeholder value for normal results. It now reflects the buffered row total.

### 4. Desktop integration coverage

The desktop suite gained `TestRowSetNavigation`, which verifies:

- `RowCount = 3` for the ordered genre sample
- last-row navigation
- previous-row navigation
- first-row navigation
- `BeforeFirstRow` behavior
- forward movement after BOF

## Important Implementation Note

Phase 21 does not attempt editable-row behavior.

The cursor now feels better for reads, but it still does not expose row update/delete cursor hooks. That remains a separate question and should not be conflated with read-navigation parity.

## Why This Slice Was Chosen

This was the highest-value next parity step because it:

- improves daily Xojo ergonomics immediately
- reduces one of the clearest differences versus the built-in drivers
- avoids speculative complexity
- keeps the public API unchanged

## Remaining Gap

Phase 21 improves read navigation, but it does not yet add:

- editable/updateable cursor behavior
- event/notification support
- PostgreSQL-style large-object objects

Those are still separate parity decisions.

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `phase_21.md`

## Final Result

Phase 21 closed with the desktop suite green:

- `172 passed`
- `0 failed`

Verified on April 7, 2026.
