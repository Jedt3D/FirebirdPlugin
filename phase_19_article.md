# Phase 19 Article

## Summary

Phase 19 is a research and design spike for SSL/TLS-style parity.

The main conclusion is simple:

- Firebird can support connection-specific encryption policy
- Firebird can support connection-specific auth plugin selection
- Firebird does not cleanly map to PostgreSQL-style per-connection certificate-file properties from the official evidence reviewed

That means the correct next implementation is Firebird-native first, not a fake PostgreSQL clone.

## What Was Investigated

This phase compared:

- Xojo's public `PostgreSQLDatabase` SSL surface
- official Firebird connection and configuration documentation
- the current plugin connection path in `sources/FirebirdDB.cpp`

The parity question was whether the Firebird plugin should expose:

- `SSLMode`
- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`

or whether Firebird's real connection model points to a different public surface.

## Key Finding

The strongest clean match is not certificate-file parity. The strongest clean match is:

- `WireCrypt`
- `AuthClientPlugins`

Official Firebird documentation shows that:

- configuration can be overridden per connection using DPB config items
- `WireCrypt` has documented connection-level values
- authentication plugin selection can be connection-specific

That is enough to justify a production-grade connection-security slice.

What the same official material does not establish cleanly is a PostgreSQL-style per-connection certificate-path model suitable for public Xojo properties.

## Recommended Direction

The next implementation phase should expose:

- `WireCrypt As String`
- `AuthClientPlugins As String`

Possible later ergonomics layer:

- `SSLMode As String` as a narrow alias over `WireCrypt`

Recommended mapping if that alias is added:

- `disable` -> `Disabled`
- `prefer` -> `Enabled`
- `require` -> `Required`

What should not be added yet:

- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`

Those properties would imply a certificate-path contract that this phase did not validate from the official Firebird connection model.

## Why This Matters

The user goal is to make the Firebird plugin feel closer to a first-class built-in Xojo driver, with PostgreSQL as the gold standard.

This phase shows that the right path is:

- copy the built-in-driver feel where it maps cleanly
- keep Firebird-native naming where the engine is genuinely different

That preserves production credibility instead of chasing superficial parity.

## Final Recommendation

Phase 19 ends with this recommendation:

1. implement `WireCrypt`
2. implement `AuthClientPlugins`
3. optionally add a limited `SSLMode` alias
4. do not add certificate-path properties until official Firebird per-connection support is proven clearly
