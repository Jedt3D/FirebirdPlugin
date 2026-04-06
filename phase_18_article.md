# Phase 18 Article

## Summary

Phase 18 adds a built-in-driver-style affected-row reporting surface to `FirebirdDatabase`:

- `AffectedRowCount As Int64`

This starts the parity-focused enhancement track by closing one of the most visible day-to-day ergonomics gaps between the Firebird plugin and Xojo's built-in database drivers.

## What Was Added

### 1. `AffectedRowCount` on `FirebirdDatabase`

The plugin now tracks the most recent non-query execution result and exposes it as a read-only Xojo property.

The property is updated by:

- direct `ExecuteSQL`
- prepared `ExecuteSQL`
- native `AddRow`

The property is intentionally preserved across `SelectSQL`, so a follow-up read does not erase the previous DML count.

### 2. Desktop integration coverage

The desktop suite gained `TestAffectedRowCount`, which verifies:

- `INSERT` reporting
- `UPDATE` reporting
- `DELETE` reporting
- prepared `ExecuteSQL` reporting
- preservation across `SelectSQL`

## Important Implementation Note

During Phase 18, the pulled Windows loader and ARM64 work temporarily regressed the macOS build by forcing parts of the modern Firebird interface layer onto a stubbed path. That broke:

- service-manager operations
- BLOB readback
- native `BOOLEAN` readback
- some Firebird 4/5/6 modern-type paths

The stable fix was to keep the real modern-interface path on platforms where `fb_c_api.h` is available, and keep the fallback stub path only for the Windows ARM64 case that still needs it.

That let Phase 18 land without sacrificing the already-complete modern-type and services work.

## Why This Slice Was Chosen

`AffectedRowCount` was the highest-value next parity step because it:

- improves normal app code immediately
- matches an expected built-in-driver workflow
- has low conceptual risk
- did not require public API churn beyond one small property

This made it the right first step before larger work such as SSL/TLS, eventing, or a streaming BLOB object model.

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `drivers-comparison.md`
- `enhancement_plan.md`
- `phase_18.md`

## Final Result

Phase 18 closed with the desktop suite green:

- `162 passed`
- `0 failed`

Verified on April 7, 2026.
