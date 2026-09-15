# TODO — known gaps (all verified against the pinned stack source and/or the wire, not assumed)

Stack pin: `abd4cee1c7f28ca8e1af4720849c4081082bbe82` (6.x @ 2026-09-10, reports 6.0.21).

## 1. AE-AC-B: no customer-facing way to generate a true ACCESS_EVENT notification

`BACnetStack_SetIntrinsicAccessEventAlgorithm` and `BACnetStack_SetAccessEventContext` are **not**
on the customer surface at this pin. `source/CASBACnetStackDLL.h` says why, verbatim, right where
the exports used to be:

> Sprint 75 - BACnetStack_SetIntrinsicAccessEventAlgorithm and BACnetStack_SetAccessEventContext
> were moved to CASBACnetStackTestToolDLL (BACnetStackTestTool_*) per PR #169 review: ACCESS_EVENT
> is exercised from BTL, not exposed as a customer-facing host API.

There is also no generic manual "send this event notification" export (grepped
`CASBACnetStackDLL.h` for `EventNotification`: only the two `AtomicRead/WriteFile` hits, both
unrelated). The only way an application-armed intrinsic algorithm generates a notification is via
one of the `BACnetStack_SetIntrinsic*Algorithm*` family, so a substitute was tried:
`BACnetStack_SetIntrinsicChangeOfStateAlgorithmUnsigned` on Access Point 1 ("Copper")'s
`Access_Event` property. **That substitute does not work either** — confirmed by actually calling
it against the running binary:

```
::CASBACnetStack::BACnetDBDevice::EnableIntrinsicChangeOfStateAlgorithmUnsigned() ... Error:
intrinsic algorithm changeOfState is not supported by objectType=[33]
```

So AE-AC-B's alarm **generation** is not implementable through the customer API today, on top of
the missing algorithm setter itself. Copper's `Access_Event` property is still fully readable (and
was made writable so a client can simulate a credential read, per the file header's demo) but no
intrinsic engine watches it and no notification is produced from a write to it. A live wire test
also found the write itself currently answers `Error(object: unknown-object)` rather than
succeeding — see item 4 below; that is a second, independent problem on top of the missing
algorithm.

**Filed:** [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044).

## 2. Access Credential / Access Rights: several REQUIRED constructed-type properties cannot be served

Verified live over the wire (`bacpypes3`, this session): `ReadProperty` of
`Access_Credential,1.Authentication_Factors` and `Access_Rights,1.Negative_Access_Rules` both
answer `Abort(other)`, not a clean value or a documented error. Root cause: both are
`BACnetARRAY[N]` of a complex constructed type (`BACnetCredentialAuthenticationFactor`,
`BACnetAccessRule`) with no dedicated typed Get callback anywhere in `CASBACnetStackDLL.h` (the
only precedent, `RegisterCallbackGetPropertyAuthenticationFactorFormat`, covers a *different*,
simpler property — `Supported_Formats`, not `Authentication_Factors`) and no generic
`GetPropertyConstructed` on the customer surface (that callback is test-tool-only — see
`source/CASBACnetStackDLL.h`'s own note, cross-referenced by every prior repo in this series that
hit the same wall: B-LSC's Life Safety Zone `Zone_Members`, B-AAC's Calendar `Date_List`). The same
applies to Access Point's `Access_Event_Time` (`BACnetTimeStamp`) and `Access_Event_Credential`
(`BACnetDeviceObjectReference`), and Access Zone's `Entry_Points`/`Exit_Points` (`BACnetLIST` of
`BACnetDeviceObjectReference`) — not read-tested individually this session for time, but the same
export-absence applies identically; treat as the same class of gap.

**Filed:** [chipkin/cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046)
(also covers items 4 and 5 below).

## 3. Event Log ("Beige"): `BACnetStack_AddEventLogObject` produces a continuous internal error log flood

Wire/log-verified this session by bisection (built and ran seven progressively-reduced
configurations of this exact binary): a device with Event Log 1 added via the documented,
customer-facing `BACnetStack_AddEventLogObject(deviceInstance, 1, 64)` call — and otherwise
*nothing else non-base added* (no Access Point/Zone/Credential/Rights, no Schedule/Calendar, no
File, no Backup/Restore) — logs, from the very first `BACnetStack_Tick()` and continuously
thereafter (dozens of times over a few seconds, never stopping):

```
::CASBACnetStack::BACnetDateTime::operator =() in file: .../source/BACnetDateTime.cpp(85) - Error: Failed to set the date
::CASBACnetStack::BACnetDateTime::operator =() in file: .../source/BACnetDateTime.cpp(89) - Error: Failed to set the time
```

Removing only the `AddEventLogObject` call (with every other object/feature in this file's full
configuration left in place) makes the flood disappear entirely; adding it back reproduces it
immediately and reliably. This isolates the trigger to `AddEventLogObject` itself, independent of
every other object type in this file (confirmed separately: Access Point/Zone/Credential/Rights,
Schedule/Calendar, File, Backup/Restore, and this file's own `GetPropertyDate`/`GetPropertyTime`
registrations were each tested removed-then-restored and are not the cause).

Functionally the device kept answering Who-Is/I-Am and ReadProperty correctly despite the flood
(verified live), and `Event_Log,1.Record_Count` read back `0` correctly — so this looks like noisy,
non-fatal internal logging rather than a functional break, but it was not practical to rule out
every downstream effect (e.g. on `Log_Buffer`/ReadRange, or on a long-running deployment's log
volume) in the time available this session.

**Filed:** [chipkin/cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045).
**Also flagged to the B-ALSC sibling task** (Wave 3, same runbook), which also adds an Event Log
object per its own card — this is very likely to reproduce there too.

## 4. Copper's `Access_Event` WriteProperty (the credential-read demo trigger) fails on the wire

`BACnetStack_SetPropertyWritable(..., accessPoint, 1, Access_Event, true)` at start-up returns
`true` (no setup error), but a live `WriteProperty` to it from `bacpypes3` answers
`Error(object: unknown-object)`, not the accepted write this file's `SetPropertyEnumerated`
callback is written to handle. Not root-caused in the time available this session — recorded here
rather than left silently broken. The `README`'s "Verify" section and support table mark this
demo path unverified rather than claiming it works. **Filed:** covered by
[chipkin/cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046).

## 5. File 1 ("Ivory") `File_Size` denies ReadProperty

A live `ReadProperty` of `File,1.File_Size` answered `Error(object: read-access-denied)` rather
than the file's current size (0, since nothing had been written to it yet in that session).
`File_Size` is a REQUIRED property per `property-profile-reference.md`'s File entry. Not
root-caused in the time available — `AtomicReadFile` against the same object also aborted
(`Abort(other)`) during an active backup session (see the Verify section); whether these two are
the same underlying cause is unconfirmed.

## 6. Calendar 1 ("Cream")'s `Date_List` — inherited, pre-existing gap

Same gap B-AAC's file header already documents against stack issue #963:
`BACnetStack_AddScheduleExceptionEventWithCalendarReference` does not resolve a Calendar's
`Date_List` at evaluation time, and there is no customer-facing way to populate `Date_List` at all.
This file uses the inline `...WithCalendarEntry` exception form instead (fully functional), exactly
as B-AAC does; Cream still exists as an object with `Date_List` `accepted` (not served) in
`docs/objects.json`.
