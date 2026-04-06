# Firebird Plugin Enhancement Plan

Last updated: April 7, 2026

This draft was refreshed after pulling `main` at commit `78cbe85`, which includes the latest Windows FB5/FB6 build updates.

## Goal

The essential goal is to make the Firebird plugin feel closer to a first-class built-in Xojo database driver.

That means prioritizing:

1. better parity with Xojo's built-in MySQL, PostgreSQL, and SQLite drivers where the mapping is technically clean
2. preserving Xojo-style `Database` / `PreparedStatement` / `RowSet` ergonomics
3. extending Firebird-specific advantages only after the parity layer is strong enough

## Current Position

The Firebird plugin is already strong in:

- core `Database` workflow
- prepared statements
- transaction support
- schema introspection
- Firebird-specific admin/service operations
- Firebird 4/5/6 modern-type fidelity

The main parity gaps identified in [drivers-comparison.md](drivers-comparison.md) are:

- `AffectedRowCount`
- SSL/TLS connection controls
- richer `RowSet` behavior
- event/notification-style support
- a dedicated BLOB object only if streaming use cases matter

## Decision Update From User Direction

Your latest guidance changes the parity target in an important way:

- app-code ergonomics come first
- deployment/security comes second
- the long-term goal is still a production-grade driver
- `PostgreSQLDatabase` is the primary reference driver for parity

That means the most important parity targets are not just the earlier generic list. They are specifically the things Xojo's PostgreSQL driver exposes publicly and that real applications notice:

- `AffectedRowCount`
- SSL/TLS controls
- notifications
- large objects

It also means `RowSet` work should be treated carefully:

- if there is a real user-visible parity gap against PostgreSQL, it matters
- if the gap is mainly against SQLite's richer local-engine behavior, it should not jump ahead of PostgreSQL-style parity items

## Strategic Priorities

## Priority 1: Built-in Driver Feel

This is the highest-value track because it most directly affects how Xojo developers experience the plugin.

### 1. `AffectedRowCount`

Priority: Highest  
Difficulty: Low to medium  
Risk: Low  
Recommended order: First

Why:

- it is a built-in-driver expectation
- it improves normal app code immediately
- it has low conceptual risk
- it does not require architectural churn

Target outcome:

- after `ExecuteSQL`, users can inspect how many rows were affected
- behavior should be documented for:
  - `INSERT`
  - `UPDATE`
  - `DELETE`
  - statement shapes where Firebird does not report a meaningful count

Recommendation:

- do this next before anything else

### 2. SSL/TLS connection controls

Priority: High  
Difficulty: Medium to high  
Risk: Medium  
Recommended order: Second, but start with a feasibility spike

Why:

- this is a real parity gap against MySQL/PostgreSQL
- it matters for production deployment credibility
- Firebird client/server SSL/TLS support has to map cleanly to something Xojo users can actually configure

Important constraint:

- this should only be implemented if the Firebird connection model supports a clean and testable Xojo API
- if the mapping is partial or fragile, this should not be rushed into the public surface

Recommended approach:

1. research Firebird SSL/TLS configuration capabilities in the client stack actually used by this plugin
2. define the smallest clean Xojo property surface
3. implement only if the semantics are stable across supported Firebird versions

Candidate Xojo API shapes:

- `SSLEnabled As Boolean`
- `SSLMode As String`
- `SSLKey As FolderItem` or `String`
- `SSLCertificate As FolderItem` or `String`
- `SSLAuthority As FolderItem` or `String`

Difference between the minimal and PostgreSQL-style approaches:

- simple on/off support only decides whether the driver should attempt TLS at all
- PostgreSQL-style support also lets the caller choose verification behavior and supply certificate material

Recommended parity target:

- prefer the PostgreSQL-style shape, not just a boolean toggle
- the smallest serious production-grade target is:
  - `SSLMode`
  - `SSLKey`
  - `SSLCertificate`
  - `SSLAuthority`

Recommendation:

- do not promise this feature until a short design spike proves it can be exposed cleanly

### 3. `RowSet` capability improvements

Priority: High  
Difficulty: Medium  
Risk: Medium  
Recommended order: Third

Why:

- this improves the feeling of "built-in driver" more than adding exotic Firebird features
- the current cursor layer is fine for forward iteration, but thinner than SQLite
- this is a general quality/perception improvement, not a niche feature

Recommended scope split:

#### Phase A: safe read-only navigation improvements

- `MoveToPreviousRow`
- first/last navigation if the cursor model can support it without breaking streaming fetch
- more complete row-position semantics

#### Phase B: optional editable row behavior

- only if Xojo's database engine callbacks and Firebird statement model make it practical
- otherwise stop after better navigation and clearly document that editing remains SQL-driven

Recommendation:

- prioritize read-only navigation first
- do not over-promise row-edit/update hooks unless the engine design supports them naturally

## Priority 2: Optional Parity Extensions

These matter, but only after the first three are in good shape.

### 4. Event / notification-style support

Priority: Medium  
Difficulty: Medium to high  
Risk: Medium to high  
Recommended order: Fourth

Why:

- this is mainly a parity play against PostgreSQL notifications
- it is useful, but not essential for most app workloads
- the eventing model is more complex than regular query work

Key question:

- should this be a direct analogue to PostgreSQL `Listen` / `Notify`, or a Firebird-native event API surface with different naming?

Recommendation:

- only do this if there is real user demand for asynchronous database events
- if implemented, keep it explicitly Firebird-native rather than pretending it is PostgreSQL-compatible

### 5. Dedicated Firebird BLOB object

Priority: Low to medium  
Difficulty: Medium  
Risk: Medium  
Recommended order: Fifth

Why:

- the plugin already supports explicit text and binary BLOB bind/read behavior
- this only becomes high-value if users need streaming or partial BLOB access
- otherwise it adds API surface without improving the built-in-driver feel much

How PostgreSQL does it:

- it exposes large-object lifecycle calls on the database object
- it uses a dedicated object for operations like:
  - `Read`
  - `Write`
  - `Seek`
  - `Tell`

Recommended parity target:

- if Firebird can support a clean streaming object, follow the PostgreSQL pattern rather than inventing a one-off helper API
- if Firebird cannot support that pattern cleanly, keep the current direct bind/read support and defer the object model

Recommendation:

- defer until there is clear evidence that streaming BLOB workflows are common

## Priority 3: Firebird Distinctive Value

When the parity track is strong enough, the next goal is to maximize what makes Firebird unique in Xojo.

That track should focus on:

1. keep extending safe service-manager operations
2. deepen transaction and admin tooling
3. preserve strong modern-type coverage

This is the right long-term differentiator because Xojo's built-in drivers do not expose comparable admin/service depth.

## Best Recommended Order

If the goal is "first-class built-in driver feel" with PostgreSQL as the gold standard, the best order is:

1. `AffectedRowCount`
2. SSL/TLS feasibility spike and API design
3. SSL/TLS implementation only if the spike succeeds
4. dedicated Firebird large-object / streaming BLOB design, modeled after PostgreSQL's large-object workflow
5. event/notification support only if Firebird's event model maps cleanly enough to a PostgreSQL-like Xojo surface
6. `RowSet` improvements after the PostgreSQL-parity items above are settled
7. then return to deeper Firebird-specific service/admin expansion

## What Not to Prioritize for Built-in Parity

Some built-in-driver features do not map well enough to Firebird to justify chasing them just for checklist parity.

These should not be priority goals right now:

- SQLite attached-database management
- SQLite WAL-specific toggles
- SQLite encryption-specific APIs
- trying to imitate PostgreSQL large-object APIs exactly
- trying to imitate PostgreSQL notification APIs exactly if Firebird's event model differs materially

The correct approach is:

- copy the built-in-driver feel where the behavior maps cleanly
- keep Firebird-native naming where the engine is genuinely different

## Suggested Implementation Waves

### Wave 1: Immediate parity wins

- `AffectedRowCount`
- SSL/TLS feasibility investigation
- PostgreSQL large-object parity investigation

Expected value:

- maximum user-visible polish per unit of effort

### Wave 2: Structural parity work

- implement SSL/TLS only if the design spike succeeds
- implement a dedicated Firebird large-object / streaming BLOB surface only if the Firebird mapping is clean and the use case is real

Expected value:

- stronger built-in-driver feel in day-to-day use

### Wave 3: Conditional advanced features

- event support if demand exists and the mapping is clean
- `RowSet` quality improvements where they provide real user-visible parity

Expected value:

- advanced capability, but only if justified by actual use

### Wave 4: Distinctive Firebird expansion

- more safe service operations
- more admin controls
- stronger transaction tooling

Expected value:

- makes the plugin uniquely strong rather than merely comparable

## Questions That Need Answers

These are the questions I need you to answer before implementation starts:

1. Is your top priority developer ergonomics for normal app code, or enterprise/deployment credibility?
   - Answer received: both, but ergonomics first and deployment/security second.
   - Result: `AffectedRowCount` is first, SSL/TLS is immediately behind it.

2. Do you want strict built-in-driver mimicry, or are you comfortable with Firebird-native method names when the engine differs?
   - I recommend Firebird-native naming where behavior is not truly equivalent.

3. For `RowSet`, do you want read-navigation parity only, or do you also want editable/updateable row behavior?
   - Answer received: match PostgreSQL level first, then add editable row behavior if it is practical.
   - Result: `RowSet` work is no longer top priority unless a real PostgreSQL-parity gap is identified.

4. Is asynchronous event support an actual requirement for your users, or only a parity wish-list item?
   - Answer received: mostly a wish-list item; do what is necessary for real-world benefit, but try to match PostgreSQL.
   - Result: keep it as a later spike, not an immediate implementation target.

5. Do you expect large/streamed BLOB workflows in real projects?
   - Answer received: yes, potentially, and PostgreSQL's model is the reference.
   - Result: large-object / streaming BLOB work moves ahead of event support.

6. For SSL/TLS, do you need:
   - simple on/off support
   - certificate-path properties
   - hostname verification / mode selection
   - or just a documented compatibility story?

## What PostgreSQL Parity Means Here

Using `PostgreSQLDatabase` as the gold standard implies these practical targets:

- `AffectedRowCount` parity is mandatory
- SSL/TLS should look more like PostgreSQL's `SSLMode`, `SSLKey`, `SSLCertificate`, and `SSLAuthority` than like a single on/off switch
- large-object support should look more like `CreateLargeObject`, `OpenLargeObject`, `DeleteLargeObject`, plus a dedicated object for `Read`, `Write`, `Seek`, and `Tell`
- event support should aim for a Xojo-style pattern similar to:
  - `Listen`
  - `Notify`
  - `StopListening`
  - `CheckForNotifications`
  - a notification event

This also changes the interpretation of `RowSet` work:

- richer `RowSet` behavior is still valuable
- but it is more of a SQLite-style quality goal unless we can identify a real PostgreSQL-visible gap
- because of that, it should not block PostgreSQL-style parity work

## My Best Recommendation

If I were choosing the best path for the next engineering pass, I would do this:

1. implement `AffectedRowCount`
2. run an SSL/TLS feasibility spike using PostgreSQL-style parity as the design target
3. if feasible, implement SSL/TLS with a small but production-credible property set
4. design a Firebird large-object / streaming BLOB API modeled on PostgreSQL's Xojo surface
5. defer event support until after that unless a real application need appears immediately
6. treat `RowSet` enhancement as a secondary polish track unless it exposes a concrete PostgreSQL-parity gap
7. then continue expanding Firebird's distinctive admin/service strengths

Why this is best:

- it improves the built-in-driver feel fastest
- it avoids taking on speculative complexity too early
- it preserves momentum on features users will actually notice
- it keeps the plugin pragmatic instead of chasing every built-in-driver feature literally

## Decision Summary

Default recommended next step:

- `AffectedRowCount`

Default recommended second step:

- SSL/TLS feasibility and API design spike

Default recommended third step:

- dedicated Firebird large-object / streaming BLOB design
