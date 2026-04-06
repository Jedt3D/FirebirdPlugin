# Phase 12 Article

## Summary

Phase 12 extends the Firebird service-manager user-management track with two focused modify-user helpers on `FirebirdDatabase`:

- `SetUserAdmin(userName As String, isAdmin As Boolean) As Boolean`
- `UpdateUserNames(userName As String, firstName As String, middleName As String, lastName As String) As Boolean`

The phase keeps the public Xojo surface narrow and testable while expanding the admin workflow beyond add/delete/password-change.

## What Was Added

### 1. Admin-flag mutation

`SetUserAdmin()` runs Firebird's `isc_action_svc_modify_user` action and sets or clears `isc_spb_sec_admin`.

This made it possible to:

- promote a temporary user to admin
- verify that the promoted user can perform an admin-only action (`AddUser`)
- demote the same user back to non-admin
- verify that the same admin-only action is rejected again

### 2. User full-name mutation

`UpdateUserNames()` also uses `isc_action_svc_modify_user`, but sends:

- `isc_spb_sec_firstname`
- `isc_spb_sec_middlename`
- `isc_spb_sec_lastname`

This adds a controlled path for updating display/profile information without introducing a broader user object or generic mutation builder.

## Implementation Files

Primary native/plugin work:

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`

Desktop integration coverage:

- `example-desktop/MainWindow.xojo_window`
- `example-desktop/App.xojo_code`

Documentation/examples:

- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`

## Verification Strategy

The final verification strategy is intentionally split by concern.

### Admin behavior

The suite verifies admin enable/disable in three ways:

- service call succeeds
- `SEC$USERS.SEC$ADMIN` shows the expected value
- a real login can or cannot perform `AddUser`, depending on the current flag

### Name updates

The suite verifies name updates through `SEC$USERS`:

- `SEC$FIRST_NAME`
- `SEC$MIDDLE_NAME`
- `SEC$LAST_NAME`

This is more reliable than parsing formatted user-display output.

## Important Mid-Phase Correction

During Phase 12, there were two related problems that had to be corrected before the phase could be closed.

### 1. Service-query regression

An experiment changed service result reading from the stable `isc_info_svc_line` path to `isc_info_svc_to_eof`.

That change caused the desktop app to terminate abruptly during test runs without a normal Xojo exception path. The implementation was reverted to the stable `isc_info_svc_line` service-query flow.

### 2. `DisplayUsers()` was the wrong readback channel for Phase 12

The original Phase 12 tests tried to verify admin/name mutation through `DisplayUsers()`. That turned out to be too fragile because the service output:

- contains formatting/control characters
- can wrap a single logical user row across multiple lines
- is designed for human display, not authoritative field verification

The final fix was to keep `DisplayUsers()` on the stable existing path and move Phase 12 verification to direct `SEC$USERS` reads plus login-based behavioral checks.

## Test Harness Improvements

Two harness changes were kept because they improved reliability:

- `App.UnhandledException` now records unexpected runtime failures to a temp log and `System.DebugLog`
- the run button wraps `RunAllTests` in a top-level `Try/Catch`

The transaction-options test was also adjusted to avoid intentionally triggering the Firebird `-817` read-only write error inside the Xojo debugger, because that negative-path probe was making interactive runs unstable even when the exception was logically handled.

## Final Result

Phase 12 closed with the desktop suite green:

- `135 passed`
- `0 failed`

Verified on April 6, 2026.
