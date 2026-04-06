# Phase 11 Article

## Summary

Phase 11 adds the next narrow user-management slice to the Firebird Xojo plugin:

- `ChangeUserPassword(userName As String, password As String) As Boolean`

This phase builds directly on the earlier service-manager security work from Phases 09 and 10:

- `DisplayUsers() As Boolean`
- `AddUser(userName As String, password As String) As Boolean`
- `DeleteUser(userName As String) As Boolean`

## What Was Implemented

### 1. Xojo-facing password-change method

`FirebirdDatabase` now exposes:

- `ChangeUserPassword(userName As String, password As String) As Boolean`

This keeps the Xojo surface intentionally narrow. It adds only password mutation for an existing user without taking on broader user-profile editing.

### 2. Native Firebird service-manager integration

The native implementation was added in the C++ layer and routed through the existing service-manager plumbing:

- service attach
- service start
- service query
- service detach

The action used is:

- `isc_action_svc_modify_user`

The request is built with the Firebird utility interface and `IXpbBuilder`, which matches the pattern already used in the earlier Services API slices.

### 3. Authentication-based verification

Phase 11 verifies behavior through real database login attempts, not only through service output.

The new desktop test proves:

- the original password works before the change
- the password change succeeds
- the old password stops working
- the new password works immediately

That matters more than only checking the service response, because it proves the mutation is actually effective at the authentication boundary.

### 4. Self-cleaning desktop integration coverage

A new desktop test was added:

- `TestChangeUserPassword`

The test creates a temporary user, changes the password, verifies both old and new credential behavior, and removes the user afterward so the suite stays rerunnable.

### 5. Phase 11 plugin-reload issue

During verification, the first Xojo run still reported that `FirebirdDatabase` had no member named `ChangeUserPassword`.

This was not a source-level implementation problem. The cause was that the installed plugin bundle had not been rebuilt cleanly with the new method metadata. The fix was:

- `make clean`
- `make`
- `make install`
- full Xojo restart

After that, the new method appeared correctly in the IDE and the test suite ran green.

## Files Changed

- `sources/FirebirdDB.h`
- `sources/FirebirdDB.cpp`
- `sources/FirebirdPlugin.cpp`
- `example-desktop/MainWindow.xojo_window`
- `README.md`
- `examples/QuickReference.xojo_code`
- `examples/FirebirdExample_API2.xojo_code`
- `firebird_xojo_detail_plan.md`
- `phase_11.md`

## Result

Phase 11 completed with the desktop suite green:

- `125 passed, 0 failed`

## Outcome for the Roadmap

After Phase 11, the plugin's first Services API slice now includes:

- backup
- restore
- database statistics
- online validation
- user display
- add user
- change user password
- delete user
- service output capture

The next logical services step is now either:

- broader user-management workflows
- broader maintenance services
