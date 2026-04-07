# Phase 22 Article

Phase 22 adds a narrow `SSLMode` alias so the Firebird plugin feels closer to Xojo's `PostgreSQLDatabase` without claiming support for certificate-validation semantics that are not actually present.

## What Changed

The plugin already had Firebird-native connection-security controls through:

- `WireCrypt As String`
- `AuthClientPlugins As String`

Phase 22 adds:

- `SSLMode As Integer`

This is deliberately a convenience wrapper over `WireCrypt`.

## Mapping

Supported values map as follows:

- `0` -> `Disabled`
- `1` -> `Enabled`
- `2` -> `Enabled`
- `3` -> `Required`

Unsupported values:

- `4`
- `5`

Those are rejected because they imply certificate-validation behavior that the driver still does not expose.

## Why This Approach

The goal was parity with Xojo's PostgreSQL driver where the behavior maps cleanly, not a fake compatibility layer.

That means:

- expose `SSLMode` only where it is honest
- keep `WireCrypt` as the Firebird-native source of truth
- reject the PostgreSQL-style values that would misrepresent actual driver behavior

## Files Updated

- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`
- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `drivers-comparison.md`
- `enhancement_plan.md`
- `firebird_xojo_detail_plan.md`

## Result

Phase 22 closed with the desktop suite green:

- `175 passed`
- `0 failed`

The remaining security-parity work is now narrower and clearer:

- certificate-path properties only if Firebird exposes a clean per-connection model
- otherwise focus next on large-object / streaming BLOB design and event support evaluation
