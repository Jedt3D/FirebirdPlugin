# Phase 20 Article

## Summary

Phase 20 adds the first Firebird-native connection-security surface to `FirebirdDatabase`:

- `WireCrypt As String`
- `AuthClientPlugins As String`

This closes the design spike from Phase 19 and gives the plugin a real, testable connection-security API instead of only a documentation plan.

## What Was Added

### 1. `WireCrypt` on `FirebirdDatabase`

The plugin now lets Xojo code request Firebird wire-encryption behavior directly.

Supported values:

- `Disabled`
- `Enabled`
- `Required`

The property maps to Firebird's per-connection config override instead of a fake cross-driver SSL property set.

### 2. `AuthClientPlugins` on `FirebirdDatabase`

The plugin now lets Xojo code define the Firebird client auth-plugin negotiation list for both ordinary attaches and service-manager control attaches.

Typical usage:

- `Srp256,Srp`

### 3. Desktop integration coverage

The desktop suite gained a focused `TestConnectionSecurityOptions` slice that verifies:

- secure TCP attach with `WireCrypt = "Required"`
- encrypted-wire readback through `RDB$GET_CONTEXT('SYSTEM', 'WIRE_ENCRYPTED')`
- rejection of invalid `WireCrypt`
- rejection of invalid `AuthClientPlugins`

## Important Implementation Note

Phase 20 applies the new properties in two places:

- normal database attach DPB
- service-manager attach SPB

That second point matters because shutdown/online control already uses a separate service-only control object. Without carrying the security properties into that attach path, the public API would have looked complete while still behaving inconsistently.

## Why This Slice Was Chosen

This was the highest-value next parity step after `AffectedRowCount` because it:

- improves production-readiness
- closes a visible gap against Xojo's built-in server drivers
- preserves honest Firebird semantics
- avoids overcommitting to certificate-path properties before Firebird proves that model cleanly

## Remaining Gap

Phase 20 does not fully match PostgreSQL's public SSL surface yet.

Still open:

- whether a limited `SSLMode` alias is worth exposing
- whether Firebird can justify PostgreSQL-style certificate-path properties cleanly

The plugin now has real connection-security controls, but they are intentionally Firebird-native.

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`
- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `drivers-comparison.md`
- `enhancement_plan.md`
- `firebird_xojo_detail_plan.md`
- `phase_20.md`

## Final Result

Phase 20 closed with the desktop suite green:

- `166 passed`
- `0 failed`

Verified on April 7, 2026.
