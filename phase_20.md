# Phase 20 Detailed Plan

## Status

Complete on April 7, 2026.

Branch:

- `feature/phase-20`

## Objective

Phase 20 implements the first Firebird-native connection-security slice identified by the Phase 19 feasibility spike.

The goal is to expose two honest Xojo-facing properties:

- `WireCrypt As String`
- `AuthClientPlugins As String`

These should feel like built-in driver configuration knobs without pretending Firebird supports PostgreSQL-style per-connection certificate-path semantics.

## Version Target

Phase 20 targets only:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Deliverables

### Deliverable 1: connection-security properties on `FirebirdDatabase`

Implement:

- `WireCrypt As String`
- `AuthClientPlugins As String`

Behavior:

- apply during ordinary TCP database attachment
- apply during service-manager control attachments
- default to empty behavior when not set
- validate `WireCrypt` values before attach

Supported `WireCrypt` values:

- `Disabled`
- `Enabled`
- `Required`

### Deliverable 2: desktop integration coverage

Add desktop tests that prove:

- a remote attach works with `WireCrypt = "Required"` and `AuthClientPlugins = "Srp256,Srp"`
- `RDB$GET_CONTEXT('SYSTEM', 'WIRE_ENCRYPTED')` confirms encrypted wire use
- invalid `WireCrypt` is rejected
- invalid `AuthClientPlugins` is rejected

## In Scope

- Xojo property exposure in the plugin class table
- DPB support for:
  - `isc_dpb_auth_plugin_list`
  - `isc_dpb_config`
- service SPB support for:
  - `isc_spb_auth_plugin_list`
  - `isc_spb_config`
- desktop integration tests
- documentation and example updates

## Out of Scope

- PostgreSQL-style `SSLKey`, `SSLCertificate`, `SSLAuthority`
- a public `SSLMode` alias
- notification/event APIs
- large-object / streaming BLOB APIs
- broader `RowSet` work

## Design Rules

### Rule 1: keep the API Firebird-native

The plugin should expose Firebird's real connection-security controls first instead of inventing PostgreSQL-shaped certificate properties without a solid Firebird mapping.

### Rule 2: apply security settings to both SQL and service control attaches

Shutdown/online control and future service-manager actions should not silently ignore connection-security settings.

### Rule 3: reject invalid `WireCrypt` values before attach

Invalid option text should fail predictably in the plugin instead of being pushed into Firebird as undefined config text.

## Success Criteria

Phase 20 is complete when:

- `WireCrypt` and `AuthClientPlugins` are exposed on `FirebirdDatabase`
- the properties affect ordinary connect and service-manager attach builders
- desktop tests prove positive and negative cases
- docs and examples are updated
- the phase article is written

## Verification

Verified with the desktop suite on April 7, 2026:

- `166 passed`
- `0 failed`
