# Phase 19 Detailed Plan

## Status

Completed as a design and feasibility spike on April 7, 2026.

Branch:

- `feature/phase-19`

## Objective

Phase 19 investigates whether the Firebird plugin can expose production-grade SSL/TLS-style connection controls in a way that feels closer to Xojo's built-in `PostgreSQLDatabase`, without inventing misleading API surface.

The purpose of this phase is not to ship the feature immediately. The purpose is to answer:

1. what Firebird officially supports per connection
2. what can be mapped cleanly into Xojo properties
3. which PostgreSQL-style SSL properties do not map cleanly
4. what the next implementation slice should actually be

## Version Target

This spike was evaluated against:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

## Research Inputs

### Xojo parity reference

The parity target remains `PostgreSQLDatabase`, which publicly exposes:

- `SSLMode`
- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`

Reference:

- https://documentation.xojo.com/api/databases/postgresqldatabase.html

### Firebird official references

Primary official references reviewed:

- Firebird configuration reference:
  - https://www.firebirdsql.org/docs/html/en/refdocs/fbconf/firebird-configuration-reference.html
- Firebird configuration reference PDF:
  - https://firebirdsql.org/docs/pdf/en/refdocs/fbconf/firebird-configuration-reference.pdf
- Firebird 3 release notes / configuration additions:
  - https://firebirdsql.org/file/documentation/release_notes/html/en/3_0/rlsnotes30.html
- Firebird 3 quick start:
  - https://firebirdsql.org/file/documentation/html/en/firebirddocs/qsg3/firebird-3-quickstartguide.html

## Official Firebird Findings

### Finding 1: Firebird supports connection-specific configuration overrides

Official Firebird documentation states that client-side configuration can be overridden for a specific connection using:

- `isc_dpb_config`
- `isc_dpb_auth_plugin_list`

This means SSL/TLS-adjacent behavior is not limited to server-global `firebird.conf`. There is a real per-connection API path.

### Finding 2: the primary connection-level encryption knob is `WireCrypt`

Official Firebird documentation exposes `WireCrypt` with these values:

- `Required`
- `Enabled`
- `Disabled`

This is the closest Firebird equivalent to a PostgreSQL-style transport-encryption mode selector.

### Finding 3: authentication plugin selection is also connection-specific

Official Firebird documentation allows connection-specific control over authentication plugin selection through:

- `AuthClient`
- `isc_dpb_auth_plugin_list`

This matters because production connectivity may depend on plugin negotiation, not only on encryption policy.

### Finding 4: Firebird documents wire encryption plugins, not PostgreSQL-style certificate-file properties

The official Firebird docs and headers clearly expose:

- wire encryption plugin architecture
- authentication plugin configuration
- config-string and DPB/SPB override mechanisms

What they do not expose with equivalent clarity is a PostgreSQL-style per-connection property set for:

- client key file path
- client certificate file path
- CA bundle / authority file path
- verification modes equivalent to PostgreSQL `verify-ca` or `verify-full`

That makes PostgreSQL-style certificate properties a weak fit for a first implementation.

## Current Plugin State

The current plugin connection path in [FirebirdDB.cpp](/Users/worajedt/Xojo%20Projects/FirebirdPlugin/sources/FirebirdDB.cpp) still builds a fixed raw DPB with:

- user name
- password
- character set
- role
- SQL dialect

There is no current support for:

- `isc_dpb_config`
- `isc_dpb_auth_plugin_list`
- any connection-level encryption policy property

## Feasibility Conclusion

### Cleanly feasible now

These are good candidates for a production-grade next implementation slice:

1. a Firebird-native wire-encryption policy property
2. a Firebird-native auth-plugin-list property

That means a realistic and honest Phase 20 target is:

- `WireCrypt As String`
- `AuthClientPlugins As String`

Supported `WireCrypt` values should match Firebird documentation exactly:

- `Disabled`
- `Enabled`
- `Required`

### Feasible only as a compatibility alias

A PostgreSQL-style `SSLMode` convenience alias is possible, but only as a thin mapping over Firebird's real model.

If added at all, it should map narrowly:

- `disable` -> `WireCrypt = Disabled`
- `prefer` -> `WireCrypt = Enabled`
- `require` -> `WireCrypt = Required`

Anything beyond that would be dishonest because Firebird does not document PostgreSQL-equivalent per-connection verification semantics.

### Not cleanly feasible from the current official evidence

These should not be added in the first SSL/TLS implementation slice:

- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`
- PostgreSQL-style verification modes beyond a simple transport-policy mapping

Reason:

- the official Firebird material reviewed in this phase does not establish a clean, supported per-connection certificate-path surface comparable to PostgreSQL's Xojo API
- exposing those properties now would risk inventing a contract the underlying client library does not actually support per connection

## Recommended Public API Direction

### Canonical Firebird-native surface

Recommended first-class properties:

- `WireCrypt As String`
- `AuthClientPlugins As String`

Potential later addition:

- `WireCompression As Boolean`

### Optional compatibility layer

Only if the built-in-driver feel is considered important enough, add:

- `SSLMode As String`

with the documented limited mapping:

- `disable`
- `prefer`
- `require`

Do not add PostgreSQL-style certificate path properties unless a later phase proves they are officially and cleanly supported per connection.

## Recommended Implementation Changes

### Native layer

The next implementation slice will need to:

1. replace the fixed `char dpb[512]` builder with a dynamic DPB builder
2. append `isc_dpb_config` entries for config-string based overrides
3. append `isc_dpb_auth_plugin_list` when `AuthClientPlugins` is set
4. store the new values in `FBDatabase`
5. expose the new properties through `FirebirdPlugin.cpp`

### Test strategy

A realistic test strategy should avoid pretending we can cryptographically inspect the wire from inside Xojo.

The best practical tests are:

1. property plumbing and connection success with default-safe values
2. guarded integration tests against a server configuration where:
   - `WireCrypt = Required` rejects `Disabled`
   - `WireCrypt = Required` allows `Required`
3. guarded plugin-negotiation tests using `AuthClientPlugins`

This likely needs environment-dependent coverage, not always-on assumptions in the main desktop suite.

## Best Recommendation

Phase 19 concludes that the best next implementation step is not a full PostgreSQL SSL clone.

The best next step is:

1. implement `WireCrypt`
2. implement `AuthClientPlugins`
3. optionally add a limited `SSLMode` alias only if it improves ergonomics without hiding Firebird semantics

The best thing to avoid is:

- faking `SSLKey`, `SSLCertificate`, or `SSLAuthority` before official Firebird per-connection support is demonstrated clearly

## Success Criteria For The Next Phase

The next implementation phase should be considered successful when:

- the plugin can request connection-specific `WireCrypt` policy
- the plugin can request connection-specific auth plugin lists
- the API is documented as Firebird-native first
- any optional `SSLMode` alias is explicitly limited and documented
- no fake certificate-path properties are introduced
