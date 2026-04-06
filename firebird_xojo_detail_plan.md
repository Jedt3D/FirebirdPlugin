# Firebird Xojo Detailed Plan

## Purpose

This document defines the reference hierarchy, feature checklist, and test plan for the Xojo Firebird plugin in this repository.

The goal is not to clone another driver API 1:1. The goal is:

- use the Firebird client SDK as the engine-level source of truth
- use Jaybird as the primary behavioral guideline for driver semantics
- adopt selected ideas from the official .NET and Python drivers where they map well to Xojo
- make gaps visible between the current Xojo implementation and the Firebird client SDK surface
- tie every local test to the Firebird SDK area it exercises

## Version Scope

Target Firebird versions for this plugin plan:

- Firebird 4.x
- Firebird 5.x
- Firebird 6.x

Out of scope for implementation planning:

- Firebird 2.5
- Firebird 3.x compatibility work
- preserving older-driver behavior when it conflicts with Firebird 4/5/6 capabilities

Practical consequence of this scope:

- `INT128` is in scope
- `DECFLOAT` is in scope
- `TIME WITH TIME ZONE` is in scope
- `TIMESTAMP WITH TIME ZONE` is in scope
- transaction, wire-protocol, and metadata decisions should be judged against Firebird 4/5/6 behavior

## Reference Hierarchy

### Gold Standard Order

1. Firebird client SDK and official Firebird documentation
2. Jaybird
3. Firebird .NET Provider
4. firebird-driver for Python
5. firebird-qa for engine-level scenario inspiration

### Why This Order

#### 1. Firebird client SDK is the engine truth

The current plugin is implemented directly on top of the legacy Firebird C API in `ibase.h` and `libfbclient`, not on top of another language binding.

Current implementation proof:

- `sources/FirebirdDB.h` defines the wrapper as a thin C++ layer over `libfbclient`
- `sources/FirebirdDB.cpp` directly calls `isc_attach_database`, `isc_start_transaction`, `isc_dsql_prepare`, `isc_dsql_execute`, `isc_dsql_fetch`, `isc_open_blob2`, `isc_create_blob2`, and related functions

That means any disagreement between Jaybird, .NET, Python, and the Firebird engine must be resolved in favor of the Firebird SDK and official Firebird docs.

#### 2. Jaybird is the primary behavioral guideline

Jaybird is the best fit for the Xojo layer because it is a mature official Firebird driver with a rich API, substantial test infrastructure, explicit metadata behavior, and strong coverage of transaction and statement semantics.

Reasons to prefer Jaybird as the primary model:

- it is the official JDBC driver
- it has a large and mature codebase and test suite
- it models connection, statement, prepared statement, result set, metadata, services, and embedded/native behavior in a way that maps better to Xojo than Python DB-API does
- its release notes and manual are explicit about behavior changes that matter to client libraries
- for Firebird 4/5/6 work, Jaybird 5 and 6 are the most relevant behavioral references

Important behavioral areas to copy from Jaybird:

- connection and auto-commit semantics
- prepared statement lifecycle
- result set behavior at end-of-fetch
- generated values and `RETURNING` behavior
- metadata and schema expectations
- embedded/native vs remote attachment handling
- transaction parameter and isolation behavior when eventually exposed

#### 3. .NET Provider is the secondary provider model

The official Firebird .NET Provider is a strong secondary reference because it is also mature, production-focused, and explicit about provider-level functionality.

Use it especially for:

- typed value handling
- schema and metadata behavior
- batching ideas
- provider test matrix design
- practical edge cases around ADO-style data access

#### 4. Python firebird-driver is useful, but not the main public API model

The official Python `firebird-driver` is valuable because:

- it is official
- it uses the newer Firebird interface-based API
- its tests are compact and readable

However, Python DB-API 2.0 is less structurally similar to Xojo's `Database` / `PreparedStatement` / `RowSet` model than JDBC is, so Python should be used mainly for:

- readable scenario tests
- transaction scenarios
- BLOB and cursor usage examples
- cross-checking newer API behavior

#### 5. firebird-qa is the best source for engine scenarios

`firebird-qa` is not the client API we should emulate, but it is a good source for:

- real engine regression scenarios
- edge-case SQL behavior
- encryption, services, and server-configuration-sensitive tests

## Upstream References

### Official Firebird

- Firebird client/server and API overview: https://www.firebirdsql.org/file/documentation/pdf/en/firebirddocs/ufb/using-firebird.pdf
- Firebird JDBC driver page: https://firebirdsql.org/en/jdbc-driver/
- Firebird .NET Provider page: https://firebirdsql.org/en/net-provider/
- Firebird Python driver page: https://firebirdsql.org/en/python-driver/
- Firebird Python driver development page: https://www.firebirdsql.org/en/devel-python-driver/
- Firebird 4 release notes: https://firebirdsql.org/file/documentation/release_notes/html/en/4_0/rlsnotes40.html
- Firebird 5 release notes: https://firebirdsql.org/file/documentation/release_notes/html/en/5_0/rlsnotes50.html
- Firebird 6 snapshots and release materials as available from FirebirdSQL official channels

### Official Driver Repositories

- Jaybird: https://github.com/FirebirdSQL/jaybird
- Jaybird manual: https://firebirdsql.github.io/jaybird-manual/jaybird_manual.html
- .NET Provider: https://github.com/FirebirdSQL/NETProvider
- Python firebird-driver: https://github.com/FirebirdSQL/python3-driver
- firebird-qa: https://github.com/FirebirdSQL/firebird-qa

## Current Xojo Plugin Architecture

### Layer 1: `FirebirdDB` C++ wrapper

Location:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`

Responsibilities:

- build DPB and connect via `isc_attach_database`
- manage transactions via `isc_start_transaction`, `isc_commit_transaction`, `isc_rollback_transaction`
- prepare and execute DSQL statements via `isc_dsql_prepare`, `isc_dsql_execute`, `isc_dsql_execute2`, `isc_dsql_fetch`
- describe columns and bind parameters via XSQLDA and `isc_dsql_describe`, `isc_dsql_describe_bind`
- inspect statement type via `isc_dsql_sql_info`
- read and write BLOBs via `isc_open_blob2`, `isc_get_segment`, `isc_create_blob2`, `isc_put_segment`
- translate Firebird status into plugin error text

### Layer 2: `FirebirdPlugin` Xojo SDK bridge

Location:

- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`

Responsibilities:

- expose `FirebirdDatabase` as a Xojo `Database` subclass
- expose `FirebirdPreparedStatement`
- bridge `Database.SelectSQL`, `ExecuteSQL`, transactions, schema methods, and row navigation to the C++ wrapper
- translate Firebird types to Xojo row/column values

### Key Design Fact

The current plugin uses the legacy `isc_*` API, not the newer interface-based API from `Interfaces.h`.

That is fine for a first-generation Xojo driver, but it matters for roadmap decisions because the newer API solves some legacy limits and better matches how the modern Python driver works. For Firebird 4/5/6, the legacy API is still usable, but newer types and newer metadata features need explicit design and tests.

## Firebird SDK Checklist

The official "Using Firebird" document divides the core API into these categories:

- Database connection control
- Transaction control
- Statement execution
- Blob functions
- Array functions
- Security functions
- Informational functions
- Type conversions

It also identifies a separate Services API.

The checklist below compares the current Xojo plugin to that surface.

### Legend

- `[x]` implemented and exposed to Xojo
- `[-]` partially implemented or implemented only internally
- `[ ]` not implemented

## Feature Checklist vs Firebird Client SDK

| Area | Representative Firebird SDK surface | Status | Notes |
| --- | --- | --- | --- |
| Database attach/detach | `isc_attach_database`, `isc_detach_database` | `[x]` | Implemented in `FBDatabase::connect` and `disconnect` |
| DPB-based authentication and charset | DPB items like `isc_dpb_user_name`, `isc_dpb_password`, `isc_dpb_lc_ctype`, `isc_dpb_sql_role_name`, `isc_dpb_sql_dialect` | `[x]` | Current Xojo properties: Host, Port, DatabaseName, UserName, Password, CharacterSet, Role, Dialect |
| Remote and local attachment | attach string and client library behavior | `[x]` | Remote and embedded-style local attachment supported by connection string construction |
| Transaction begin/commit/rollback | `isc_start_transaction`, `isc_commit_transaction`, `isc_rollback_transaction` | `[x]` | Current Xojo API exposes begin/commit/rollback |
| Auto-start transaction for statement execution | driver-level behavior on top of transaction API | `[x]` | Implemented by `ensureTransaction()` and plugin auto-commit behavior |
| Custom TPB / isolation exposure | transaction parameter buffer control | `[-]` | Phase 04 exposes common TPB-backed options for isolation, access mode, and lock timeout through `BeginTransactionWithOptions`, but not arbitrary TPB composition |
| Dynamic SQL prepare | `isc_dsql_allocate_statement`, `isc_dsql_prepare` | `[x]` | Used for both `SelectSQL` and prepared statements |
| Parameter metadata | `isc_dsql_describe_bind` | `[x]` | Used internally |
| Result metadata | `isc_dsql_describe` | `[x]` | Used internally to map output columns |
| Statement type inspection | `isc_dsql_sql_info` | `[x]` | Used to distinguish executable procedures |
| Execute immediate | `isc_dsql_execute_immediate` | `[x]` | Used by `ExecuteSQL` without returned rows |
| Prepared statement execute | `isc_dsql_execute`, `isc_dsql_execute2` | `[x]` | Exposed through `FirebirdPreparedStatement.ExecuteSQL` and `SelectSQL` |
| Forward-only fetch | `isc_dsql_fetch` | `[x]` | Current rowset implementation is forward-only |
| Scrollable cursor support | driver-level or alternative statement handling | `[ ]` | Not implemented |
| Text binding | XSQLDA buffers for `SQL_TEXT` / `SQL_VARYING` | `[x]` | Exposed to Xojo |
| Integer binding | XSQLDA buffers for integer types | `[x]` | Exposed to Xojo |
| Floating / scaled numeric binding | XSQLDA buffers for `SQL_FLOAT`, `SQL_DOUBLE`, scaled numerics | `[x]` | Exposed to Xojo through `Double` and internal scaled handling |
| Boolean binding | `SQL_BOOLEAN` or legacy numeric fallback | `[x]` | Exposed to Xojo |
| NULL binding | `sqlind = -1` | `[x]` | Exposed to Xojo |
| Date binding | `SQL_TYPE_DATE` | `[x]` | Exposed through `FirebirdPreparedStatement.Bind(index, value As DateTime)` |
| Time binding | `SQL_TYPE_TIME` | `[x]` | Exposed through `FirebirdPreparedStatement.Bind(index, value As DateTime)` |
| Timestamp binding | `SQL_TIMESTAMP` | `[x]` | Exposed through `FirebirdPreparedStatement.Bind(index, value As DateTime)` |
| BLOB binding | `isc_create_blob2`, `isc_put_segment` | `[x]` | Exposed through `BindTextBlob` and `BindBinaryBlob` |
| Text BLOB read | `isc_open_blob2`, `isc_get_segment` with transliteration BPB | `[x]` | Implemented internally |
| Binary BLOB read | `isc_open_blob2`, `isc_get_segment` | `[x]` | Implemented internally |
| Array API | `isc_array_*` | `[ ]` | Not implemented |
| Database info API | `isc_database_info` | `[x]` | Exposed to Xojo through typed database info helper methods |
| Transaction info API | `isc_transaction_info` | `[x]` | Phase 03 added inspection helpers and Phase 04 added explicit TPB-backed transaction controls |
| Request API / BLR style APIs | low-level request functions | `[ ]` | Not implemented and probably out of scope for Xojo v1 |
| Event API | `isc_event_*` | `[ ]` | Not implemented |
| Security/user management core API | security-related APIs | `[-]` | Connection security properties (`WireCrypt`, `AuthClientPlugins`) and service-manager user operations are implemented; broader security surface is still open |
| Services API | backup, restore, statistics, validation, user management, trace, etc. | `[-]` | Phase 17 extends the first slice to backup, restore, database statistics, online validation, sweep, limbo-transaction listing, limbo recovery, sweep-interval control, shutdown/online control, user display, add/delete/password-change, admin-flag mutation, name mutation, and verbose service output |
| Type conversions for text and date/time | helper functions and driver mapping | `[x]` | Current plugin maps common legacy types to Xojo values |
| `INT128` support | newer data type support | `[x]` | Exposed through `StringValue` and type-aware string binding |
| `DECFLOAT` support | newer data type support | `[x]` | Exposed through `StringValue` and type-aware string binding |
| `TIME WITH TIME ZONE` support | newer data type support | `[x]` | Exposed through `StringValue` and type-aware string binding |
| `TIMESTAMP WITH TIME ZONE` support | newer data type support | `[x]` | Exposed through `StringValue` and type-aware string binding |
| Interface-based API | `fb_get_master_interface()`, `Interfaces.h` | `[ ]` | Current plugin is entirely on legacy `isc_*` API |

## Phase Progress Checklist

This table tracks implementation progress against the plan through the currently completed phases.

| Phase | Scope | Status | Verification | Branch | Commit | Article |
| --- | --- | --- | --- | --- | --- | --- |
| 01 | database info helpers, temporal binds, BLOB binds, Firebird-specific SQL coverage stabilization | Complete | `68 passed, 0 failed` | `feature/phase-01` | `fa844a2` | `phase_01_article.md` |
| 02 | Firebird 4/5/6 modern type mapping and round-trip coverage | Complete | `78 passed, 0 failed` | `feature/phase-02` | `fe81a41` | `phase_02_article.md` |
| 03 | transaction info helpers | Complete | `86 passed, 0 failed` | `feature/phase-03` | `3760c15` | `phase_03_article.md` |
| 04 | explicit TPB-backed transaction controls | Complete | `96 passed, 0 failed` | `feature/phase-04` | `fccc82f` | `phase_04_article.md` |
| 05 | generated-key convenience through native Xojo `AddRow` hooks | Complete | `100 passed, 0 failed` | `feature/phase-05` | `fef5dcd` | `phase_05_article.md` |
| 06 | Services API backup and restore slice | Complete | `107 passed, 0 failed` | `feature/phase-06` | `9910e8c` | `phase_06_article.md` |
| 07 | Services API database statistics slice | Complete | `110 passed, 0 failed` | `feature/phase-07` | `66d6c00` | `phase_07_article.md` |
| 08 | Services API online validation slice | Complete | `113 passed, 0 failed` | `feature/phase-08` | `303fa7c` | `phase_08_article.md` |
| 09 | Services API read-only user display slice | Complete | `116 passed, 0 failed` | `feature/phase-09` | `8cec51e` | `phase_09_article.md` |
| 10 | Services API basic user-mutation slice | Complete | `120 passed, 0 failed` | `feature/phase-10` | `f46905c` | `phase_10_article.md` |
| 11 | Services API password-change slice | Complete | `125 passed, 0 failed` | `feature/phase-11` | `95d8583` | `phase_11_article.md` |
| 12 | Services API admin-flag and user-name mutation slice | Complete | `135 passed, 0 failed` | `feature/phase-12` | `30fbe52` | `phase_12_article.md` |
| 13 | Services API database sweep slice | Complete | `138 passed, 0 failed` | `feature/phase-13` | `4afbda6` | `phase_13_article.md` |
| 14 | Services API limbo-transaction listing slice | Complete | `141 passed, 0 failed` | `feature/phase-14` | `6311245` | `phase_14_article.md` |
| 15 | Services API sweep-interval property slice | Complete | `144 passed, 0 failed` | `feature/phase-15` | `6c88875` | `phase_15_article.md` |
| 16 | Services API limbo-recovery slice | Complete | `149 passed, 0 failed` | `feature/phase-16` | `4311cc4` | `phase_16_article.md` |
| 17 | Services API shutdown / online control slice | Complete | `157 passed, 0 failed` | `feature/phase-17` | `924d213` | `phase_17_article.md` |
| 18 | built-in-parity affected-row reporting slice | Complete | `162 passed, 0 failed` | `feature/phase-18` | `83b4baa` | `phase_18_article.md` |
| 19 | SSL/TLS feasibility and API design spike | Complete | design spike complete | `feature/phase-19` | `b681d5f` | `phase_19_article.md` |
| 20 | Firebird-native connection-security properties slice | Complete | `166 passed, 0 failed` | `feature/phase-20` | `588c217` | `phase_20_article.md` |

## Current Xojo Feature Snapshot

### Implemented and exposed now

- connect / disconnect
- remote and local file attachment patterns
- `SelectSQL`
- `ExecuteSQL`
- `Database.AddRow(...)` and generated-key return through Xojo's native insert hook
- prepared statements
- string, integer, double, boolean, `DateTime`, and null binding through Xojo
- text BLOB and binary BLOB binding through Xojo prepared statements
- row iteration and column access
- transaction begin, commit, rollback
- transaction info helpers for active transaction inspection
- explicit TPB-backed transaction options for isolation, read-only/read-write, and lock timeout
- schema helpers: tables, columns, indexes
- database info helpers backed by `isc_database_info`
- Firebird-native connection-security properties: `WireCrypt`, `AuthClientPlugins`
- Services API first slice for backup, restore, database statistics, online validation, sweep, limbo-transaction listing, limbo recovery, sweep-interval control, shutdown/online control, user display, add/delete/password-change, admin-flag mutation, name mutation, and verbose service output
- common legacy type mapping: integer, bigint, float/double, numeric/decimal, varchar/char, blob, date, time, timestamp, boolean
- Firebird 4/5/6 modern type mapping: `INT128`, `DECFLOAT`, `TIME WITH TIME ZONE`, `TIMESTAMP WITH TIME ZONE` via string semantics
- text and binary BLOB reads
- error code and message propagation

### Implemented internally but not fully exposed to Xojo

- statement type inspection
- XSQLDA-level metadata details

### Missing compared to the broader Firebird SDK

- broader Services API beyond backup/restore/statistics/validation/sweep/limbo-list/limbo-recovery/sweep-interval/shutdown-online/user-display and the current user-mutation slice
- Events API
- Array API
- modern interface-based API
- broader transaction management APIs beyond current inspection helpers
- richer `RETURNING` handling helpers beyond the native generated-key convenience path
- batch APIs beyond repeated prepared statement execution

## Target Feature List for Firebird 4/5/6

This is the intended feature set for the Xojo plugin within the current scope.

### Core features

- connect and disconnect
- remote and local attachment
- `SelectSQL`
- `ExecuteSQL`
- prepared statements
- rowset iteration and column access
- transaction begin, commit, rollback
- transaction info helpers for active transaction inspection
- explicit transaction options backed by Firebird TPB
- schema helpers for tables, columns, and indexes
- database info helpers backed by `isc_database_info`
- Services API first slice for backup, restore, database statistics, online validation, sweep, limbo-transaction listing, limbo recovery, sweep-interval control, shutdown/online control, user display, add/delete/password-change, admin-flag mutation, name mutation, and verbose service output

### Type support

- string types
- integer and bigint types
- scaled `NUMERIC` and `DECIMAL`
- floating-point types
- `BOOLEAN`
- `DATE`
- `TIME`
- `TIMESTAMP`
- text BLOB and binary BLOB
- `INT128`
- `DECFLOAT`
- `TIME WITH TIME ZONE`
- `TIMESTAMP WITH TIME ZONE`

### Firebird-specific SQL behavior

- `RETURNING`
- `EXECUTE BLOCK`
- executable stored procedures
- embedded/local attachment scenarios

### Deferred features

- broader Services API beyond backup/restore/statistics/validation/sweep/limbo-list/limbo-recovery/sweep-interval/shutdown-online/user-display and the current user-mutation slice
- Events API
- Array API
- BLR/request-style APIs

## Guidance for What to Copy From Upstream

### Copy from Jaybird first

Use Jaybird as the behavioral model for:

- auto-commit semantics
- prepared statement lifecycle
- result set end-of-data behavior
- generated values and `RETURNING`
- metadata consistency
- database-info semantics and naming
- embedded/native vs remote semantics
- transaction options and isolation design when exposed
- future service-manager API design

### Copy from .NET Provider second

Use the .NET Provider for:

- typed reader/writer behavior
- schema metadata expectations
- database metadata and provider info helper ideas
- provider test matrix structure
- practical edge cases around nulls, numerics, booleans, and provider-level behavior

### Copy from Python third

Use `firebird-driver` and `firebird-qa` for:

- readable scenario tests
- newer interface API behavior cross-checks
- engine-level edge cases
- BLOB and transaction scenario examples

## Local Test Inventory and C-SDK Coverage

Primary local test source:

- `example-desktop/MainWindow.xojo_window`

Current suite entry points:

- `TestConnect`
- `TestConnectBadCredentials`
- `TestDatabaseInfo`
- `TestSelectSQL`
- `TestSelectSQLColumnTypes`
- `TestSelectSQLUnicodeThai`
- `TestSelectSQLWithParams`
- `TestRowSetIteration`
- `TestRowSetColumnAccess`
- `TestExecuteSQL`
- `TestTransaction`
- `TestTransactionRollback`
- `TestTransactionInfo`
- `TestTransactionOptions`
- `TestPreparedStatementSelect`
- `TestPreparedStatementExecute`
- `TestPreparedStatementBindTypes`
- `TestPreparedStatementBindTemporalTypes`
- `TestPreparedStatementBindBlobs`
- `TestPreparedStatementBindNull`
- `TestNativeBooleanRoundTrip`
- `TestScaledNumericRoundTrip`
- `TestInt128RoundTrip`
- `TestDecFloatRoundTrip`
- `TestTimeWithTimeZoneRoundTrip`
- `TestTimestampWithTimeZoneRoundTrip`
- `TestDatabaseAddRow`
- `TestDatabaseAddRowWithReturnValue`
- `TestServicesBackupRestore`
- `TestDatabaseStatistics`
- `TestValidateDatabase`
- `TestSweepDatabase`
- `TestListLimboTransactions`
- `TestRecoverLimboTransactions`
- `TestSetSweepInterval`
- `TestShutdownOnlineControl`
- `TestDisplayUsers`
- `TestAddDeleteUser`
- `TestChangeUserPassword`
- `TestSetUserAdmin`
- `TestUpdateUserNames`
- `TestReturningClause`
- `TestExecuteBlock`
- `TestExecuteProcedure`
- `TestEmbeddedConnect`
- `TestTableSchema`
- `TestFieldSchema`
- `TestDatabaseIndexes`
- `TestErrorHandling`
- `TestMultipleConnections`
- `TestLargeResultSet`
- `TestEmptyResultSet`
- `TestNullValues`
- `TestBeginCommit`

## Existing Local Tests Mapped to Firebird SDK Areas

| Local test case | What it checks | Firebird SDK area exercised | Representative API/functions |
| --- | --- | --- | --- |
| `TestConnect` | valid attach and detach | Database connection control | `isc_attach_database`, `isc_detach_database` |
| `TestConnectBadCredentials` | auth failure path and error propagation | Database connection control, error handling | `isc_attach_database`, status vector capture |
| `TestDatabaseInfo` | server version, page size, SQL dialect, ODS, read-only flag | Informational functions | `isc_database_info` |
| `TestSelectSQL` | scalar select query | Statement execution | `isc_dsql_allocate_statement`, `isc_dsql_prepare`, `isc_dsql_execute`, `isc_dsql_fetch` |
| `TestSelectSQLColumnTypes` | integer, string, integer, double readback | Statement execution, type conversions | DSQL prepare/fetch plus XSQLDA output mapping |
| `TestSelectSQLUnicodeThai` | UTF-8 text round-trip and transliteration expectations | Database connection control, type conversions | DPB charset handling plus text fetch |
| `TestSelectSQLWithParams` | positional parameter binding for integer and double | Statement execution, parameter metadata | `isc_dsql_describe_bind`, execute with XSQLDA input |
| `TestRowSetIteration` | repeated fetch and ordered traversal | Statement execution | `isc_dsql_fetch` |
| `TestRowSetColumnAccess` | column access by name and index, column count | Statement metadata | `isc_dsql_describe` and plugin column mapping |
| `TestExecuteSQL` | insert, update, delete | Statement execution, auto-commit behavior | `isc_dsql_execute_immediate` or prepared execute path plus commit |
| `TestTransaction` | committed insert persists | Transaction control | `isc_start_transaction`, `isc_commit_transaction` |
| `TestTransactionRollback` | rolled-back insert disappears | Transaction control | `isc_start_transaction`, `isc_rollback_transaction` |
| `TestTransactionInfo` | active transaction state, id, isolation, access mode, lock timeout | Transaction control, informational functions | `isc_start_transaction`, `isc_transaction_info`, `isc_commit_transaction` |
| `TestTransactionOptions` | explicit TPB-backed transaction options and read-only enforcement | Transaction control | `isc_start_transaction` with TPB options, `isc_transaction_info`, `isc_commit_transaction`, `isc_rollback_transaction` |
| `TestPreparedStatementSelect` | prepared select with bound parameter | Statement execution, parameter metadata | allocate/prepare/describe_bind/execute/fetch |
| `TestPreparedStatementExecute` | prepared insert | Statement execution | allocate/prepare/execute |
| `TestPreparedStatementBindTypes` | string, int64, double, boolean binds and readback | Statement execution, type conversions | XSQLDA input binding for text/numeric/boolean |
| `TestPreparedStatementBindTemporalTypes` | `DATE`, `TIME`, and `TIMESTAMP` prepared binds and readback | Statement execution, type conversions | XSQLDA input/output mapping for legacy temporal types |
| `TestPreparedStatementBindBlobs` | text and binary BLOB bind/fetch behavior | Blob functions, statement execution | `isc_create_blob2`, `isc_put_segment`, `isc_open_blob2`, `isc_get_segment` |
| `TestPreparedStatementBindNull` | null indicator for prepared parameter | Statement execution, null semantics | XSQLVAR `sqlind` handling |
| `TestNativeBooleanRoundTrip` | native Firebird `BOOLEAN` storage and readback | Statement execution, type conversions | XSQLDA input/output mapping for `SQL_BOOLEAN` |
| `TestScaledNumericRoundTrip` | scaled `NUMERIC` / `DECIMAL` round-trip behavior | Statement execution, type conversions | XSQLDA input/output mapping for scaled numerics |
| `TestInt128RoundTrip` | `INT128` and `NUMERIC(38,4)` round-trip behavior | Statement execution, type conversions | utility-interface conversion plus XSQLDA mapping |
| `TestDecFloatRoundTrip` | `DECFLOAT(16)` and `DECFLOAT(34)` round-trip behavior | Statement execution, type conversions | utility-interface conversion plus XSQLDA mapping |
| `TestTimeWithTimeZoneRoundTrip` | `TIME WITH TIME ZONE` round-trip behavior | Statement execution, type conversions | utility-interface conversion plus XSQLDA mapping |
| `TestTimestampWithTimeZoneRoundTrip` | `TIMESTAMP WITH TIME ZONE` round-trip behavior | Statement execution, type conversions | utility-interface conversion plus XSQLDA mapping |
| `TestDatabaseAddRow` | native Xojo `Database.AddRow(...)` insertion | Statement execution | insert builder plus DSQL execute |
| `TestDatabaseAddRowWithReturnValue` | generated-key return through Xojo `AddRow` hook | Statement execution, metadata lookup | `RETURNING` plus primary-key metadata SQL |
| `TestServicesBackupRestore` | server-side backup, restore, and restored database readback | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestDatabaseStatistics` | service-manager database statistics and output capture | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestValidateDatabase` | online database validation and diagnostic output capture | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestSweepDatabase` | database sweep service execution and post-sweep queryability | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestListLimboTransactions` | limbo-transaction listing on a clean database and output capture | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestRecoverLimboTransactions` | targeted limbo-recovery calls with safe clean-database verification | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestSetSweepInterval` | reversible database sweep-interval property update with `MON$DATABASE` readback | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestShutdownOnlineControl` | denied-new-attachments shutdown and online restoration using a separate service-control object | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestDisplayUsers` | read-only user display and output capture | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestAddDeleteUser` | add-user and delete-user mutation with display-based readback | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach` |
| `TestChangeUserPassword` | password mutation with old/new credential verification | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach`, `isc_attach_database` |
| `TestSetUserAdmin` | admin-flag mutation with behavioral and `SEC$USERS` verification | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach`, `isc_attach_database` |
| `TestUpdateUserNames` | full-name mutation with `SEC$USERS` verification | Services API | `isc_service_attach`, `isc_service_start`, `isc_service_query`, `isc_service_detach`, `isc_dsql_prepare`, `isc_dsql_execute`, `isc_dsql_fetch` |
| `TestReturningClause` | Firebird `RETURNING` row behavior | Statement execution | execute-with-output via DSQL and output XSQLDA |
| `TestExecuteBlock` | `EXECUTE BLOCK` result row behavior | Statement execution | DSQL prepare/execute/fetch on Firebird-specific SQL |
| `TestExecuteProcedure` | executable stored procedure singleton-row behavior | Statement execution, statement-type inspection | `isc_dsql_sql_info`, execute-with-output |
| `TestEmbeddedConnect` | hostless/local attachment path behavior | Database connection control | `isc_attach_database` with local attachment string |
| `TestTableSchema` | tables metadata via system tables | Statement execution, informational behavior implemented by SQL | custom SQL over `RDB$` metadata tables |
| `TestFieldSchema` | columns metadata via system tables | Statement execution, informational behavior implemented by SQL | custom SQL over `RDB$` metadata tables |
| `TestDatabaseIndexes` | index metadata via system tables | Statement execution, informational behavior implemented by SQL | custom SQL over `RDB$` metadata tables |
| `TestErrorHandling` | clean initial error state and valid query behavior | Error/status handling | status capture and reset |
| `TestMultipleConnections` | two simultaneous attachments | Database connection control | multiple DB handles and independent statement contexts |
| `TestLargeResultSet` | repeated fetch over join result | Statement execution | repeated fetch and row materialization |
| `TestEmptyResultSet` | EOF / `AfterLastRow` behavior | Statement execution | fetch returning no rows |
| `TestNullValues` | null output conversion to Xojo values | Type conversions, null semantics | output XSQLDA `sqlind` handling |
| `TestBeginCommit` | multiple DML statements in one explicit transaction | Transaction control | begin / multiple execute / commit |

## Existing Coverage Gaps

These are notable areas not covered by the current local desktop suite:

- concurrent transaction visibility / isolation tests
- statement reuse after multiple execute cycles
- broader Services API beyond backup/restore/statistics/validation/sweep/limbo-list/limbo-recovery/sweep-interval/shutdown-online/user-display and the current user-mutation slice
- generated keys abstraction beyond the native Xojo `AddRow` callback

## Planned Test Additions Inspired by Jaybird, .NET, and Python

### Phase 1: Fill gaps in already-implemented legacy API behavior

Status: completed on April 6, 2026.

These should be added first because the underlying C++ layer already supports most of them.

| Planned test | Primary upstream inspiration | C-SDK area | Why it matters |
| --- | --- | --- | --- |
| Prepared date bind round-trip | Jaybird, .NET | Statement execution, type conversions | Complete in Phase 01 |
| Prepared time bind round-trip | Jaybird, .NET | Statement execution, type conversions | Complete in Phase 01 |
| Prepared timestamp bind round-trip | Jaybird, .NET | Statement execution, type conversions | Complete in Phase 01 |
| Text BLOB insert and fetch | Jaybird, Python | Blob functions | Complete in Phase 01 |
| Binary BLOB insert and fetch | .NET, Python | Blob functions | Complete in Phase 01 |
| Native `BOOLEAN` round-trip | Jaybird, .NET | Statement execution, type conversions | Complete in Phase 01 |
| `NUMERIC` / `DECIMAL` scale matrix | Jaybird, .NET | Type conversions | Complete in Phase 01 |
| Stored procedure execute and result variables | Jaybird | Statement execution | Complete in Phase 01 |
| `RETURNING` result behavior | Jaybird | Statement execution | Complete in Phase 01 |
| `EXECUTE BLOCK` basic execution | Python, firebird-qa | Statement execution | Complete in Phase 01 |
| Embedded/local attachment test | Jaybird native/embedded, Python | Database connection control | Complete in Phase 01 |

### Phase 2: Add Firebird 4/5/6 modern type support

Status: completed on April 6, 2026.

| Feature | Primary upstream inspiration | Current state | Action |
| --- | --- | --- | --- |
| `INT128` output mapping | Jaybird, .NET | Complete | Exposed as `StringValue` at the Xojo boundary |
| `DECFLOAT(16)` / `DECFLOAT(34)` output mapping | Jaybird, .NET | Complete | Exposed as `StringValue` at the Xojo boundary |
| `TIME WITH TIME ZONE` output mapping | Jaybird, .NET | Complete | Exposed as normalized textual value |
| `TIMESTAMP WITH TIME ZONE` output mapping | Jaybird, .NET | Complete | Exposed as normalized textual value |
| Type-aware string binding for modern types | Jaybird, .NET, Python | Complete | Converts textual Xojo input into Firebird wire structs using utility interfaces |
| Modern-type desktop coverage | Jaybird, .NET | Complete | Round-trip tests added for all in-scope Firebird 4/5/6 types |

### Phases 3-18: Expand toward broader Firebird SDK surface

| Feature | Primary upstream inspiration | Current state | Priority |
| --- | --- | --- | --- |
| database info helpers | Jaybird, .NET | Complete in Phase 01 | Done |
| transaction info helpers | Jaybird, .NET | Complete in Phase 03 | Done |
| explicit transaction controls | Jaybird, .NET | Complete in Phase 04 with typed TPB-backed options | Done |
| generated-key / `AddRow` convenience | Jaybird, Xojo database API | Complete in Phase 05 through native `AddRow` callbacks | Done |
| affected-row reporting | Xojo built-in driver expectations | Complete in Phase 18 through `AffectedRowCount` on non-query execution paths | Done |
| Services API wrapper | Jaybird ServiceManager, .NET docs | Phase 17 completes the first backup/restore/statistics/validation/sweep/limbo-list/limbo-recovery/sweep-interval/shutdown-online/user-display/add-delete-password-admin-profile/output slice | In progress by slices |
| Event API wrapper | Jaybird event APIs | Missing | Medium |
| Array API | Firebird SDK only | Missing | Low |
| move from legacy API to interface-based API | Python firebird-driver, Firebird 3+ docs | Missing | Long-term decision |

## Progress Summary Through Phase 20

Planned through Phase 20 and now complete:

- affected-row reporting for non-query execution paths
- SSL/TLS feasibility and API design spike
- Firebird-native `WireCrypt` / `AuthClientPlugins` connection properties
- database info helpers
- Firebird 4/5/6 modern type support
- transaction info helpers
- explicit TPB-backed transaction options
- generated-key convenience through native `AddRow`
- Services API first slice:
  - backup
  - restore
  - database statistics
- online validation
- sweep
- limbo-transaction listing
- limbo recovery
- sweep-interval control
- shutdown / online control
- user display
- add user
- change user password
- set user admin
- update user names
- delete user
- service output capture

Still outside completed scope after Phase 20:

- PostgreSQL-style `SSLMode` alias and certificate-path properties
- dedicated large-object / streaming BLOB surface
- broader user-management workflows
- broader maintenance/repair services
- event API
- array API
- interface-based API migration

## Explicit Checklist for Next Engineering Pass

### Keep as-is

- continue using the legacy `isc_*` API for the short term
- continue using the Xojo `Database` / `PreparedStatement` / `RowSet` shape
- keep schema introspection implemented as SQL over `RDB$` tables

### Do next

- improve `RowSet` capability where it creates a real built-in-driver parity gain
- decide whether a limited `SSLMode` alias is worth exposing
- evaluate a dedicated Firebird large-object / streaming BLOB API
- decide whether Firebird event APIs are worth exposing in the public Xojo surface

### Defer until after the above

- broader Services API
- Event API
- Array API
- interface-based API rewrite

## Decision on Legacy API vs Interface API

Short-term recommendation:

- keep the current plugin on the legacy API
- use Jaybird behavior as the semantic model
- use Python `firebird-driver` as a future reference if the plugin ever migrates to the interface-based API
- optimize the roadmap for Firebird 4/5/6 only, instead of carrying compatibility baggage for older engine generations

Reason:

- the current plugin already has a coherent legacy-API implementation
- the highest-risk issues right now are feature exposure and test coverage, not the API family choice
- migrating API families before the current behavior is fully covered would add too much churn

Long-term note:

- if the plugin needs first-class Firebird 4/5/6 type support and broader modern API features, evaluate a phased move toward the interface-based API exposed from `Interfaces.h`

## Working Rule for Future Implementation

For each new Xojo-visible feature:

1. confirm the Firebird SDK behavior first
2. check Jaybird for expected driver semantics
3. check .NET Provider for provider-style data handling and tests
4. check Python/firebird-qa for compact scenario tests
5. add or extend a Xojo desktop test that proves the behavior locally

## Source Files in This Repository Relevant to the Plan

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.h`
- `sources/FirebirdPlugin.cpp`
- `examples/FirebirdExample_API2.xojo_code`
- `examples/QuickReference.xojo_code`
- `example-desktop/MainWindow.xojo_window`
