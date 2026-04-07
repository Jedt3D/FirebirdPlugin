# Firebird vs Xojo Built-in Database Drivers

Last updated: April 7, 2026

## Scope

This report compares the current Firebird plugin in this repository against Xojo's built-in database drivers from a technical Xojo API point of view:

- `MySQLCommunityServer`
- `PostgreSQLDatabase`
- `SQLiteDatabase`

The comparison is based on:

- the current Firebird plugin public surface in [sources/FirebirdPlugin.cpp](sources/FirebirdPlugin.cpp) and [README.md](README.md)
- Xojo's official documentation for `Database`, `MySQLCommunityServer`, `PostgreSQLDatabase`, and `SQLiteDatabase`

The goal is to answer three questions:

1. Which parts are already on par with the built-in drivers?
2. Which parts are still lacking?
3. Which parts are actually stronger than the built-in drivers?

## Executive Summary

The Firebird plugin is already on par with Xojo's MySQL and PostgreSQL drivers for normal `Database`-style application work:

- connect
- `SelectSQL`
- `ExecuteSQL`
- `AffectedRowCount`
- `Prepare`
- `AddRow`
- explicit transactions
- schema introspection through `Tables`, `TableColumns`, `TableIndexes`

It is also stronger than Xojo's built-in drivers in one important area: Firebird-specific admin and service-manager operations. The current plugin exposes backup, restore, validation, sweep, limbo inspection/recovery, sweep interval, shutdown/online control, and user-management actions directly as Xojo methods.

The main areas where it still trails the built-ins are:

- no PostgreSQL-style certificate-path SSL/TLS surface
- no PostgreSQL-style large-object lifecycle semantics
- no SQLite-style local-engine utilities such as `BackUp`, encryption, attached databases, or WAL controls
- a thinner `RowSet` implementation than SQLite

## 1. Common Xojo `Database` API Parity

These are the core APIs that matter for most Xojo apps.

| Xojo API surface | Firebird plugin | MySQLCommunityServer | PostgreSQLDatabase | SQLiteDatabase | Notes |
| --- | --- | --- | --- | --- | --- |
| `Connect` | Yes | Yes | Yes | Yes | Same Xojo workflow |
| `IsConnected` | Yes | Yes | Yes | Yes | Same Xojo workflow |
| `Close` | Yes | Yes | Yes | Yes | Same Xojo workflow |
| `SelectSQL` | Yes | Yes | Yes | Yes | Same Xojo workflow |
| `ExecuteSQL` | Yes | Yes | Yes | Yes | Same Xojo workflow |
| `Prepare` | Yes | Yes | Yes | Yes | Same Xojo workflow, driver-specific prepared statement class |
| `AddRow(table, row)` | Yes | Yes | Yes | Yes | Same Xojo API |
| `AddRow(table, row, idColumnName)` | Yes | Yes | Yes | Yes | Same Xojo API |
| `BeginTransaction` | Yes | Yes | Yes | Yes | Same Xojo API |
| `CommitTransaction` | Yes | Yes | Yes | Yes | Same Xojo API |
| `RollbackTransaction` | Yes | Yes | Yes | Yes | Same Xojo API |
| `Tables` | Yes | Yes | Yes | Yes | Same Xojo API |
| `TableColumns(tableName)` | Yes | Yes | Yes | Yes | Same Xojo API |
| `TableIndexes(tableName)` | Yes | Yes | Yes | Yes | Same Xojo API |
| `Tag` / `MetaData` style storage | Partial | Yes | Yes | Yes | Firebird has driver properties plus inherited database object behavior, but does not expose every built-in convenience property exactly the same way |

### Verdict

For ordinary Xojo database code, the Firebird plugin is already in the same usage tier as the built-ins.

If an app mainly depends on:

- CRUD
- parameterized queries
- transactions
- `RowSet`
- schema discovery

then the Firebird plugin is effectively on par.

## 2. Prepared Statements and Type Binding

| Capability | Firebird plugin | MySQL | PostgreSQL | SQLite | Assessment |
| --- | --- | --- | --- | --- | --- |
| Driver-specific prepared statement class | `FirebirdPreparedStatement` | `MySQLPreparedStatement` | `PostgreSQLPreparedStatement` | `SQLitePreparedStatement` | Parity in design |
| `Bind(String)` | Yes | Yes | Yes | Yes | Parity |
| `Bind(Int64)` | Yes | Yes | Yes | Yes | Parity |
| `Bind(Double)` | Yes | Yes | Yes | Yes | Parity |
| `Bind(Boolean)` | Yes | Yes | Yes | Yes | Parity |
| `Bind(DateTime)` | Yes | Engine-dependent | Engine-dependent | Engine-dependent | Firebird plugin explicitly documents `DATE`, `TIME`, `TIMESTAMP` support |
| `BindNull` | Yes | Built-in prepared statement behavior | Built-in prepared statement behavior | Built-in prepared statement behavior | Parity in capability |
| Explicit text BLOB bind helper | Yes | No equivalent public helper | No equivalent public helper | SQLite uses BLOB APIs instead | Firebird advantage |
| Explicit binary BLOB bind helper | Yes | No equivalent public helper | PostgreSQL has large-object APIs, not same bind helper | SQLite uses BLOB APIs instead | Firebird advantage for inline prepared binding |
| Modern engine-specific types | `INT128`, `DECFLOAT`, `TIME/TIMESTAMP WITH TIME ZONE` | Not relevant | Not relevant in same way | Not relevant in same way | Firebird-specific advantage |

### Verdict

The Firebird prepared-statement layer is stronger than "minimum parity". It is especially good in:

- temporal binding
- explicit text/binary BLOB binding
- modern Firebird 4/5/6 type support

## 3. Firebird Plugin-Specific Additions

These methods are not part of the standard Xojo `Database` API and represent plugin-added Firebird-specific surface:

- `AffectedRowCount() As Int64`
- `WireCrypt As String`
- `AuthClientPlugins As String`
- `SSLMode As Integer`
- `ServerVersion() As String`
- `PageSize() As Integer`
- `DatabaseSQLDialect() As Integer`
- `ODSVersion() As String`
- `IsReadOnly() As Boolean`
- `HasActiveTransaction() As Boolean`
- `TransactionID() As Int64`
- `TransactionIsolation() As String`
- `TransactionAccessMode() As String`
- `TransactionLockTimeout() As Integer`
- `BeginTransactionWithOptions(isolation As String, readOnly As Boolean, lockTimeout As Integer) As Boolean`
- `BackupDatabase(...)`
- `RestoreDatabase(...)`
- `DatabaseStatistics()`
- `ValidateDatabase()`
- `SweepDatabase()`
- `ListLimboTransactions()`
- `CommitLimboTransaction(...)`
- `RollbackLimboTransaction(...)`
- `SetSweepInterval(...)`
- `ShutdownDenyNewAttachments(...)`
- `BringDatabaseOnline()`
- `DisplayUsers()`
- `AddUser(...)`
- `ChangeUserPassword(...)`
- `SetUserAdmin(...)`
- `UpdateUserNames(...)`
- `DeleteUser(...)`
- `CreateBlob() As FirebirdBlob`
- `OpenBlob(rowset As RowSet, column As String) As FirebirdBlob`
- `BindBlob(index As Integer, value As FirebirdBlob)`
- `FirebirdBlob.BlobId() As String`
- `FirebirdBlob.Length() As Int64`
- `FirebirdBlob.Position() As Int64`
- `FirebirdBlob.IsOpen() As Boolean`
- `FirebirdBlob.Read(count As Integer) As MemoryBlock`
- `FirebirdBlob.Write(value As MemoryBlock) As Boolean`
- `FirebirdBlob.Seek(offset As Int64, whence As Integer) As Int64`
- `FirebirdBlob.Close() As Boolean`
- `LastServiceOutput() As String`

This is the single biggest area where the Firebird plugin is ahead of Xojo's built-in drivers.

## 4. Built-in Driver Features Firebird Does Not Yet Match

### MySQLCommunityServer

Public features in the Xojo docs that the Firebird plugin does not currently mirror:

- `SecureAuth`
- `SSLEnabled`
- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`
- `SSLAuthorityFolder`
- `SSLCipher`
- `TimeOut`

Technical impact:

- MySQL has a stronger public connection-security surface
- Firebird now has a smaller Firebird-native connection-security API through `WireCrypt` and `AuthClientPlugins`, but it still does not match MySQL's certificate and cipher controls

### PostgreSQLDatabase

Public features in the Xojo docs that the Firebird plugin does not currently mirror:

- `AppName`
- `MultiThreaded`
- `SSLKey`
- `SSLCertificate`
- `SSLAuthority`
- `CreateLargeObject`
- `OpenLargeObject`
- `DeleteLargeObject`

Technical impact:

- Firebird now matches PostgreSQL's practical notification workflow with `Listen`, `StopListening`, `Notify`, `CheckForNotifications`, and a shipped `ReceivedNotification` event, but it still does not match PostgreSQL's sender-id / payload semantics
- Firebird now has a real streaming blob object through `FirebirdBlob`, `CreateBlob`, `OpenBlob`, and `BindBlob`, but PostgreSQL is still ahead on exact large-object lifecycle semantics
- PostgreSQL is still ahead on certificate-validation semantics and SSL material controls

### SQLiteDatabase

Public features in the Xojo docs that the Firebird plugin does not currently mirror:

- `DatabaseFile`
- in-memory constructor behavior
- `BackUp`
- `AddDatabase`
- `RemoveDatabase`
- `EncryptionKey`
- `Encrypt`
- `Decrypt`
- `LoadExtensions`
- `ThreadYieldInterval`
- `Timeout`
- `WriteAheadLogging`
- `LibraryVersion`

Technical impact:

- SQLite is much stronger as an embedded/local-engine toolkit
- Firebird now matches the `CreateBlob` / `OpenBlob` method names, but SQLite is still stronger on local-engine tuning and deployment ergonomics
- SQLite has richer engine-tuning features exposed at the Xojo level
- SQLite has stronger local deployment ergonomics than Firebird

## 5. Areas Where Firebird Is Ahead

### A. Service-manager operations

The built-in Xojo drivers do not expose an equivalent public operational surface for:

- backup / restore
- database statistics
- validation
- sweep
- limbo listing and recovery
- sweep interval updates
- shutdown / online control
- user administration

From a technical Xojo API point of view, Firebird is ahead here.

### B. Transaction introspection and control

The Firebird plugin exposes:

- transaction id
- isolation reporting
- access-mode reporting
- lock-timeout reporting
- explicit TPB-backed transaction options

This is more database-control surface than Xojo publicly exposes for MySQL, PostgreSQL, or SQLite.

### C. Firebird type fidelity

The plugin already handles Firebird-specific modern types in a deliberate way:

- `INT128`
- `DECFLOAT(16)`
- `DECFLOAT(34)`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

That gives it stronger engine-native fidelity than a generic driver layer.

### D. UTF-8 behavior compared with MySQL

Xojo's MySQL docs explicitly warn that returned strings may not have a defined encoding. The Firebird plugin has explicit `CharacterSet` handling and local test coverage for UTF-8/Thai text.

From a practical text-handling point of view, the Firebird plugin is currently in a better place than the built-in MySQL driver.

## 6. Areas Where Firebird Is Still Behind

### A. Security / connection options

Compared with MySQL and PostgreSQL, Firebird currently lacks:

- PostgreSQL-style certificate-path SSL/TLS controls
- richer connection tuning options
- driver-level compatibility knobs

### B. Advanced engine-native features

Compared with PostgreSQL and SQLite, Firebird currently lacks:

- PostgreSQL-style large-object lifecycle semantics
- SQLite-style encryption and attached-database management

### C. `RowSet` richness

The current Firebird cursor implementation is still thinner than SQLite's, but the gap is smaller now. Internally, the plugin cursor now supports buffered previous/first/last row navigation and a real `RowCount`, but it still does not expose row-edit/update/delete hooks.

That means:

- it is now good for normal forward iteration and common read-navigation
- it is still not as feature-rich as SQLite's dynamic `RowSet`
- the remaining gap is mainly editable cursor behavior, not basic navigation

### D. Bundling / ecosystem maturity

SQLite is part of the base Xojo experience, and MySQL/PostgreSQL are standard built-in plugin targets. Firebird is still a custom plugin that must be shipped, versioned, and refreshed separately.

That is not a technical SQL weakness, but it is a real platform maturity gap.

## 7. Practical Technical Ranking

### For normal app CRUD work

Ranking:

1. SQLite
2. PostgreSQL / MySQL / Firebird are all in the same practical band

Reason:

- Firebird already has the Xojo APIs most real apps use
- the remaining gaps are mostly advanced or engine-specific

### For database administration from inside Xojo

Ranking:

1. Firebird
2. SQLite
3. PostgreSQL
4. MySQL

Reason:

- Firebird has the broadest explicit admin/service surface in the Xojo-facing API
- SQLite has strong local-engine utilities
- PostgreSQL and MySQL built-ins are more focused on query access than operational control

### For embedded / single-file local database workflows

Ranking:

1. SQLite
2. Firebird
3. PostgreSQL / MySQL

Reason:

- SQLite is purpose-built for this
- Firebird can do embedded/local attachment, but it does not yet expose SQLite-level convenience APIs

### For advanced Xojo-database integration features

Ranking:

1. PostgreSQL
2. SQLite
3. Firebird
4. MySQL

Reason:

- PostgreSQL still has richer notification payload semantics and large objects
- SQLite has rich local-engine APIs
- Firebird now has strong admin APIs plus shipped event notifications, but still has fewer app-integration extras than PostgreSQL or SQLite

## 8. Bottom-Line Assessment

### On par with the built-ins

The Firebird plugin is already on par for:

- `Database`-style query execution
- affected-row reporting for non-query execution paths
- prepared statements
- transactions
- schema inspection
- `AddRow`
- normal Xojo app development workflows

### Still lacking

The Firebird plugin is still behind the built-ins in:

- PostgreSQL-style certificate controls
- PostgreSQL-style large-object lifecycle semantics
- SQLite backup/encryption/attachment/WAL tooling
- editable `RowSet` behavior

### Stronger than the built-ins

The Firebird plugin is stronger in:

- database service-manager operations
- transaction observability
- Firebird engine-specific administrative control
- Firebird 4/5/6 type fidelity

## 9. Recommended Next Steps for Parity

If the goal is to make the Firebird plugin feel closer to a first-class built-in driver, the highest-value next steps are:

1. Revisit editable `RowSet` behavior only if it produces a real Xojo-visible gain
2. Add certificate-path properties only if Firebird exposes a clean per-connection model for them
3. Revisit PostgreSQL-style large-object lifecycle semantics only if the shipped Firebird-native blob surface proves insufficient in real projects

Phase 18 closes the earlier `AffectedRowCount` gap.
Phase 20 closes the first connection-security parity slice through `WireCrypt` and `AuthClientPlugins`.
Phase 21 closes the forward-only `RowSet` navigation gap through buffered read navigation and real `RowCount`.
Phase 22 closes the narrow PostgreSQL-style `SSLMode` alias gap through an honest wrapper over `WireCrypt`.
Phase 23 closes the PostgreSQL large-object parity investigation with a no-go decision on direct API mimicry.
Phase 24 closes the event feasibility question with a go decision on limited Firebird-native notification support.
Phase 25 closes that event gap with a shipped `Listen` / `StopListening` / `Notify` / `CheckForNotifications` surface and the `ReceivedNotification` event.
Phase 26 closes the Firebird-native blob foundation gap with `FirebirdBlob`, `CreateBlob`, `OpenBlob`, and `BindBlob`.

If the goal is to maximize Firebird's distinctive value instead, the better path is:

1. keep extending safe service-manager operations
2. deepen transaction/admin tooling
3. preserve strong modern-type coverage

## Sources

Official Xojo docs:

- `Database`: https://documentation.xojo.com/api/databases/database.html
- `MySQLCommunityServer`: https://documentation.xojo.com/api/databases/mysqlcommunityserver.html
- `PostgreSQLDatabase`: https://documentation.xojo.com/api/databases/postgresqldatabase.html
- `SQLiteDatabase`: https://documentation.xojo.com/api/databases/sqlitedatabase.html
- Included plugins: https://documentation.xojo.com/getting_started/using_the_ide/included_plugins.html

Current Firebird plugin implementation:

- [sources/FirebirdPlugin.cpp](sources/FirebirdPlugin.cpp)
- [sources/FirebirdDB.h](sources/FirebirdDB.h)
- [README.md](README.md)
