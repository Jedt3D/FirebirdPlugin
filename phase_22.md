# Phase 22 Detailed Plan

Status: Complete on April 7, 2026  
Branch: `feature/phase-22`

## Goal

Add a narrow PostgreSQL-style `SSLMode` alias so the Firebird plugin feels closer to Xojo's built-in `PostgreSQLDatabase` without pretending to support certificate-validation semantics that Firebird does not expose cleanly through this driver.

## Scope

Phase 22 is intentionally limited to an honest convenience alias over the already-complete Firebird-native security controls:

- `WireCrypt As String`
- `AuthClientPlugins As String`
- new `SSLMode As Integer`

This phase does not add:

- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`
- hostname verification
- certificate validation modes

## Public API

Add:

- `SSLMode As Integer`

Supported values:

- `0` -> `WireCrypt = "Disabled"`
- `1` -> `WireCrypt = "Enabled"`
- `2` -> `WireCrypt = "Enabled"`
- `3` -> `WireCrypt = "Required"`

Rejected values:

- `4`
- `5`

Reason:

- those values imply certificate-validation semantics that this driver does not implement

## Implementation Files

- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`

## Desktop Verification

The desktop suite adds and verifies:

- `SSLMode connect`
- `SSLMode readback`
- `Unsupported SSLMode rejected`

Existing security checks remain:

- `WireCrypt/AuthClientPlugins connect`
- `WireCrypt readback`
- `Invalid WireCrypt rejected`
- `Invalid AuthClientPlugins rejected`

## Success Criteria

Phase 22 is complete when:

1. `SSLMode` can be configured from Xojo code
2. supported values map cleanly to `WireCrypt`
3. unsupported values are rejected explicitly
4. the public docs explain that `SSLMode` is only an alias, not a full PostgreSQL-style certificate API
5. the full desktop suite remains green

## Verification Result

Verified with the desktop suite:

- `175 passed`
- `0 failed`

## Follow-on Work

Phase 22 closes the narrow `SSLMode` parity gap. The remaining PostgreSQL-style security gap is certificate material and validation behavior, which should stay deferred until Firebird exposes a clean per-connection model for it in this plugin.
