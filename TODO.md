# TODO - known gaps

Stack Pin: `6.x` @ `64b2cfac5` (6.0.23). Every entry was re-checked at this pin
on 2026-10-15, on the wire where it can be observed.

**This is a Blocked Example.** The B-ACC Profile Function (ANSI/ASHRAE
135-2024 L.6.2: "performs authentication and authorization for physical
access. It supports the modification of its Access Credential and Access
Rights objects by another device", with access-event notifications) needs
items 1 and 2, and both are stack gaps the owner ruled on 2026-10-15 are not
promoted to the customer interface for now (stack PR #3937, commit
`aeaa1a37e`). When either closes, re-check here first.

## 1. AE-AC-B: no customer-facing way to generate an ACCESS_EVENT notification

**Stack issue:** [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044) (open; not promoted).

At the pin, `BACnetStackTestTool_SetIntrinsicAccessEventAlgorithm` and
`BACnetStackTestTool_SetAccessEventContext` are declared only in
`CASBACnetStackTestToolDLL.h`; `CASBACnetStackDLL.h` has only the comment
recording that they were moved there. There is no generic "send this event
notification" export, and the generic substitute
`BACnetStack_SetIntrinsicChangeOfStateAlgorithmUnsigned` on Access Point 1
("Copper")'s `Access_Event` is rejected by the stack (found at 6.0.21 by calling
it):

```
::CASBACnetStack::BACnetDBDevice::EnableIntrinsicChangeOfStateAlgorithmUnsigned() ... Error:
intrinsic algorithm changeOfState is not supported by objectType=[33]
```

So nothing in this example generates an event notification of any kind.
`Access_Event` is a plain readable/writable property (the write is accepted and
reads back at this pin). #2044's design notes say the engine already builds for
customers and only the wrappers are test-tool; promotion is waiting on a
signature fix (no time-delay parameters for ACCESS_EVENT, cl. 13.3.12; the
credential belongs in the per-event context).

## 2. DS-ACUC-B / DS-ACSC-B: the authorization-data properties cannot be served

**Stack issue:** [chipkin/cas-bacnet-stack#2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150) (open; not promoted; part A of [#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046)).

The discovery Wire Test reads every property in every object's
`Property_List`. At this pin six required properties do not read as a value
(Windows and Linux, the same answers):

| Object | Property | Answer |
|---|---|---|
| Access Credential 1 "Coral" | `Assigned_Access_Rights` (256) | Error property / unknown-property |
| Access Credential 1 "Coral" | `Authentication_Factors` (257) | Error property / unknown-property |
| Access Point 1 "Copper" | `Access_Doors` (246) | Error property / unknown-property |
| Access Point 1 "Copper" | `Access_Event_Credential` (249) | Abort, reason other |
| Access Rights 1 "Cyan" | `Negative_Access_Rules` (288) | Error property / unknown-property |
| Access Rights 1 "Cyan" | `Positive_Access_Rules` (302) | Error property / unknown-property |

They are BACnetARRAYs of constructed types (and one BACnetDeviceObjectReference)
with no typed Get callback on the customer interface. They are what says which
credential carries which factors and which rights, and which rules allow whom
through which point: without them another device cannot read or configure the
authorization data, which is the B-ACC Profile Function and the substance of
DS-ACUC-B (K.1.34) and DS-ACSC-B (K.1.36). The Wire Test reports each as a
`[KNOWN GAP]`; when one starts reading as a value it prints `[GAP CLOSED]`.

(#2150's own table, from `b790b97dd`, lists whole-array reads of these as
VALUE_NOT_INITIALIZED; at `64b2cfac5` they answer UNKNOWN_PROPERTY. Same gap,
different error code.)

## 3. Calendar 1 ("Cream") still uses the generic object

Not a stack gap any more: stack #963 is closed, and the stack's Calendar engine
is on the customer interface at this pin (`BACnetStack_AddCalendarObject`,
`AddCalendarDateEntry`, `AddScheduleExceptionEventWithCalendarReference`). This
example still adds Cream with `BACnetStack_AddObject` and serves
`Present_Value` = false itself, and Saffron uses an inline exception date.
Moving Cream to the engine is example work for the Profile-functional rework.

## 4. Backup/restore payload round-trip not re-tested

At 6.0.21 an `AtomicReadFile` against File 1 ("Ivory") during an active backup
session aborted. Stack #2152 (closed) covered it with the `File_Size` defect;
`File_Size` now reads as a value. The full startBackup / AtomicReadFile /
endBackup and startRestore / AtomicWriteFile / endRestore cycle has not been
re-run at this pin.
