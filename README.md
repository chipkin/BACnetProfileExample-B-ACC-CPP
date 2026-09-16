# BACnet B-ACC (Access Control Controller) - C++ example

A BACnet Testing Laboratories Access Control Controller (B-ACC) device profile example, built on
the [CAS BACnet Stack](https://www.chipkin.com/cas-bacnet-stack/). Seeded from B-LSC (Life Safety
Controller) with the life-safety objects/callbacks removed entirely and the access family added in
their place - see `main.cpp`'s file header for the full reasoning and every verified stack
limitation this profile ran into.

## Versions

- APP_VERSION: 1.0.0
- common/ (vendored helper): 2.5.0
- CAS BACnet Stack: 6.0.21 (`6.x` @ `abd4cee1`), linked as a STATIC library

## What is a B-ACC (Access Control Controller) profile?

An Access Control Controller manages door hardware and credential readers: it presents Access
Door, Access Point, Access Zone, Access Credential and Access Rights objects; reports access
events; schedules door unlock windows; and supports backup/restore of its configuration. Annex
L.5's mandatory capabilities for a B-ACC:

| BIBB | What it means here |
|---|---|
| DS-RP-B, DS-RPM-B | Answer ReadProperty / ReadPropertyMultiple |
| DS-WP-B, DS-WPM-B | Accept WriteProperty / WritePropertyMultiple |
| DS-COV-B | Answer SubscribeCOV (Bronze's Present_Value, Flax's Update_Time) |
| DS-ACUC-B | Access Control Unlock Command - WriteProperty Cobalt's Present_Value |
| DS-ACSC-B | Access Control Supervisory Command - WriteProperty Copper's Authorization_Mode |
| AE-AC-B | Report access alarms/events - **see the Verify section; a real, verified stack gap** |
| AE-ACK-B, AE-INFO-B | Accept AcknowledgeAlarm, answer GetEventInformation |
| AE-EL-I-B | Event Log interface (ReadRange of Beige's Log_Buffer) |
| SCHED-I-B | Schedule/Calendar initiate (Saffron unlocks Cobalt on a schedule) |
| DM-BR-B | Backup and Restore (Ivory carries the payload) |
| DM-DDB-A,B, DM-DOB-B | Who-Is/I-Am (answer + initiate), Who-Has/I-Have |
| DM-DCC-B | DeviceCommunicationControl |
| DM-TS-B / DM-UTC-B | TimeSynchronization / UTCTimeSynchronization |
| DM-RD-B | ReinitializeDevice (also drives the backup/restore state machine) |

## The device this example creates

| Object | Name | Notes |
|---|---|---|
| Device 389009 | Rainbow | `--deviceID` overrides |
| Analog Input 1 | Bronze | REAL, degrees Celsius; read-only; COV-subscribable |
| Binary Input 1 | Emerald | active/inactive; read-only |
| Multi-State Input 1 | Hot Pink | state 1..3; read-only |
| Access Door 1 | Cobalt | BACnetDoorValue; commandable (lock/unlock) - DS-ACUC-B, SCHED-I-B target |
| Credential Data Input 1 | Flax | AuthenticationFactor; Update_Time COV-subscribable |
| Access Point 1 | Copper | Access_Event demo target; Authorization_Mode writable (DS-ACSC-B) |
| Access Zone 1 | Ebony | Occupancy_State |
| Access Credential 1 | Coral | Credential_Status |
| Access Rights 1 | Cyan | Enable; Global_Identifier writable |
| Event Log 1 | Beige | AE-EL-I-B |
| Schedule 1 | Saffron | writes Cobalt at priority 12 |
| Calendar 1 | Cream | Date_List not populatable (inherited gap, see TODO.md) |
| File 1 | Ivory | stream access, backup/restore payload |
| Notification Class 1 | Crimson | routes Copper's alarms (see TODO.md #1) |
| Network Port 1 | Vermilion | BACnet/IP |

## What this example does NOT do yet

See `TODO.md` for the full, wire-verified list with filed stack issues. Summary:

1. **AE-AC-B alarm generation is not implementable through the customer API at this pin.**
   The dedicated intrinsic access-event algorithm setter is not on the customer surface (it was
   moved to the stack's internal test-tool-only interface per a past PR review), and the generic
   `SetIntrinsicChangeOfStateAlgorithmUnsigned` substitute rejects `objectType=accessPoint` -
   confirmed by calling it. [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)
2. Several REQUIRED constructed-type properties (`Authentication_Factors`, `Assigned_Access_Rights`,
   `Access_Event_Time`, `Access_Event_Credential`, `Entry_Points`, `Exit_Points`,
   `Negative_Access_Rules`, `Positive_Access_Rules`) have no customer-facing Get callback - confirmed
   on the wire for `Authentication_Factors` and `Negative_Access_Rules`.
   [chipkin/cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046)
3. **Adding an Event Log object causes a continuous, non-fatal internal log flood** from the first
   `Tick()` - isolated by bisection to that one call, independent of every other object in this
   file. The device keeps answering requests correctly despite it.
   [chipkin/cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045)
4. A live WriteProperty to Copper's `Access_Event` (the credential-read demo trigger) currently
   answers `Error(unknown-object)` rather than reaching this file's callback; not root-caused.
5. A live `AtomicReadFile` against Ivory during an active backup session aborted; not root-caused
   in the time available this session. `File_Size` reads correctly.
6. Cream's `Date_List` cannot be populated (inherited gap, stack issue #963 - same as B-AAC).

## Before you ship

Change `VENDOR_IDENTIFIER`, `VENDOR_NAME`, `MODEL_NAME`, `DEVICE_DESCRIPTION`, and pick a real
`DCC_PASSWORD` if you need one. See `main.cpp`'s "Device identity" block.

## Requires the CAS BACnet Stack (licensed product)

This example links the [CAS BACnet Stack](https://www.chipkin.com/cas-bacnet-stack/), a
commercially licensed product, as a git submodule (`submodules/cas-bacnet-stack`, private - your
GitHub account needs read access, or CI's `CAS_STACK_PAT` secret). It is not included by this
CC0-licensed example itself.

## Link mode: STATIC

This example builds against a prebuilt static library (`CASBACnetStack_x64_Release.lib` /
`libCASBACnetStack_x64_Release.a`), built by the stack's own project files via
`tools/build-stack-static.sh`. The adapter also offers a SOURCE mode as a fallback (compiling the
stack's `source/` directly into the executable); this example is not shipped that way.

## Build

```bash
# from the series root (one level up from this repo)
tools/build-stack-static.sh BACnetProfileExample-B-ACC-CPP
```

```powershell
cd BACnetProfileExample-B-ACC-CPP
cmake -B build -S . -DCAS_BACNET_STACK_LINK=STATIC
cmake --build build --config Release
```

Expected output (`--port 47821`, non-default so it does not collide with another example on the
same host):

```
BACnet Access Control Controller (B-ACC) Example - C++ v1.0.0
CAS BACnet Stack version: 6.0.21.0
Common helper (common/) version: 2.5.0
FYI: Listening for BACnet/IP on UDP port 47821 (Network Port 1).
FYI: Device 389009 ("Rainbow") ready. Vendor ID 389. Press 'h' for help.
```

## Verify

Verified live this session with a real `bacpypes3` client, not just a local process check:

- Who-Is - I-Am from 389009.
- ReadProperty of every base object plus Cobalt (`Present_Value`/`Door_Status`/`Lock_Status`),
  Flax (`AuthenticationFactor`, `Update_Time`), Copper (`Access_Event`, `Authorization_Mode`),
  Ebony (`Occupancy_State`), Coral (`Credential_Status`), the Event Log's `Record_Count`, and
  Ivory's `File_Size`.
- **DS-ACUC-B**: `WriteProperty(Cobalt.Present_Value, unlock, priority 8)` accepted; a follow-up
  read confirmed `Present_Value = unlock` and `Lock_Status = unlocked`.
- **DM-BR-B**: `ReinitializeDevice(startBackup)` accepted (SimpleAck);
  `Backup_And_Restore_State` read back `idle -> performing-abackup`; `ReinitializeDevice(endBackup)`
  accepted, state returned to `idle`. This is the state-machine transition the profile requires.
- **Not verified / found broken this session** (see TODO.md): a full `AtomicReadFile`/`AtomicWriteFile`
  payload round-trip during backup; the STARTRESTORE/`AtomicWriteFile`/ENDRESTORE half of the cycle;
  a live credential-read - ACCESS_EVENT notification (blocked by TODO.md #1); the constructed-type
  properties in TODO.md #2.

## What's in this repository

- `main.cpp` - the example itself.
- `CMakeLists.txt` - build configuration (STATIC link).
- `common/` - the series' vendored helper (UDP transport, time sync, keyboard, version banner).
- `submodules/cas-bacnet-stack` - the CAS BACnet Stack (git submodule, private).
- `docs/objects.json` - source for the generated Objects and properties block below.
- `TODO.md` - every known gap, verified against the pin and/or the wire, with filed stack issues.
- `.github/workflows/release.yml` - CI: build + smoke test on every push; publish binaries and
  `metrics.json` on a `v*.*.*` tag.

## Objects and properties

<!-- OBJECTS-PROPERTIES:BEGIN (generated by tools/gen-objects-properties.py from docs/objects.json - do not edit here) -->
Every object this example creates, and every REQUIRED property of each (per ANSI/ASHRAE 135-2024 clause 12 and the stack's `docs/property-profile-reference.md`), plus the optional properties the example turns on. **Served by** says who answers a ReadProperty: the **stack** generates it, or the **app** serves it from a `GetProperty*` callback in `main.cpp`. A ⚠ row is a required property the app does not serve and the stack would fill with a default - that is a defect, not a feature.

### Analog Input 1 "Bronze" - REAL, degrees Celsius; starts at 21.5. Present_Value is DS-COV-B subscribable; the up/down key feeds subscribers via BACnetStack_UpdateValue

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | Real | app | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Out_Of_Service | Boolean | app | no |
| Units | BACnetEngineeringUnits | app | no |

### Binary Input 1 "Emerald" - starts active

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | BACnetBinaryPV | app | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Out_Of_Service | Boolean | app | no |
| Polarity | BACnetPolarity | app | no |

### Multi-state Input 1 "Hot Pink" - state 1 of 3: On, Off, Auto

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | Unsigned | app | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Out_Of_Service | Boolean | app | no |
| Number_Of_States | Unsigned | app | no |
| State_Text *(optional, enabled)* | BACnetARRAY[N] of CharacterString | app | no |

### Access Door 1 "Cobalt" - DS-ACUC-B (canonical: B-ACDC). commandable BACnetDoorValue (lock/unlock); 16-slot Priority_Array, Relinquish_Default lock(0); Door_Status/Lock_Status/Secured_Status derived from the effective Present_Value. WriteProperty-verified on the wire: unlock -> Present_Value=unlock, Lock_Status=unlocked. Also SCHED-I-B's write target (Saffron)

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | BACnetDoorValue | stack | yes |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Reliability | BACnetReliability | app | no |
| Out_Of_Service | Boolean | app | no |
| Priority_Array | BACnetARRAY[16] of BACnetOptionalDoorValue | stack | no |
| Relinquish_Default | BACnetDoorValue | app | no |
| Door_Status *(optional, enabled)* | BACnetDoorStatus | app | no |
| Lock_Status *(optional, enabled)* | BACnetLockStatus | app | no |
| Secured_Status *(optional, enabled)* | BACnetDoorSecuredStatus | app | no |
| Door_Pulse_Time | Unsigned | app | no |
| Door_Extended_Pulse_Time | Unsigned | app | no |
| Door_Open_Too_Long_Time | Unsigned | app | no |

### Credential Data Input 1 "Flax" - canonical pattern: B-ACCR. AuthenticationFactor (weigand demo badge) via GetPropertyOctetString + GetPropertyAuthenticationFactorFormat for Supported_Formats; Update_Time (NOT Present_Value) is DS-COV-B subscribable, per B-ACCR's file header. WriteProperty-verified: RP returns a real AuthenticationFactor and Update_Time=08:00:00.00 at start-up

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | BACnetAuthenticationFactor | app | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Reliability | BACnetReliability | app | no |
| Out_Of_Service | Boolean | app | no |

### Access Point 1 "Copper" - AE-AC-B / DS-ACSC-B. VERIFIED GAP (TODO.md #1, chipkin/cas-bacnet-stack#2044): SetIntrinsicAccessEventAlgorithm/SetAccessEventContext are test-tool-only, AND the generic SetIntrinsicChangeOfStateAlgorithmUnsigned substitute rejects objectType=accessPoint - confirmed by calling it. So no intrinsic engine is armed here and no notification is produced; Access_Event is a plain read/write property only. VERIFIED GAP (TODO.md #2/#4, chipkin/cas-bacnet-stack#2046): Access_Event_Time/Access_Event_Credential have no servable path (BACnetTimeStamp/BACnetDeviceObjectReference, no typed callback); a live WriteProperty to Access_Event currently answers Error(unknown-object) rather than reaching this file's callback - root cause not found in the time available. Authorization_Mode write (DS-ACSC-B supervisory command) verified accepted at start-up (authorize/denyAll)

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Reliability | BACnetReliability | app | no |
| Out_Of_Service | Boolean | app | no |
| Authentication_Status | BACnetAuthenticationStatus | app | no |
| Active_Authentication_Policy | Unsigned | app | no |
| Number_Of_Authentication_Policies | Unsigned | app | no |
| Authorization_Mode | BACnetAuthorizationMode | app | yes |
| Access_Event | BACnetAccessEvent | app | yes |
| Access_Event_Tag | Unsigned | app | no |
| Access_Event_Time | BACnetTimeStamp | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Access_Event_Credential | BACnetDeviceObjectReference | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### Access Zone 1 "Ebony" - Occupancy_State (normal) wire-verified. VERIFIED GAP (TODO.md #2, chipkin/cas-bacnet-stack#2046): Entry_Points/Exit_Points (required BACnetLIST of BACnetDeviceObjectReference) have no servable path on the customer surface - same class of gap as Access Credential/Rights below, not individually wire-tested this session

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Global_Identifier | Unsigned32 | app | yes |
| Occupancy_State | BACnetAccessZoneOccupancyState | app | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Reliability | BACnetReliability | app | no |
| Out_Of_Service | Boolean | app | no |
| Entry_Points | BACnetLIST of BACnetDeviceObjectReference | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Exit_Points | BACnetLIST of BACnetDeviceObjectReference | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### Access Credential 1 "Coral" - Credential_Status (active) wire-verified. VERIFIED GAP (TODO.md #2, chipkin/cas-bacnet-stack#2046): Authentication_Factors (required BACnetARRAY of BACnetCredentialAuthenticationFactor) answers Abort(other) on live ReadProperty - confirmed on the wire, not assumed. Activation_Time/Expiration_Time (BACnetDateTime) and Assigned_Access_Rights (BACnetARRAY of BACnetAssignedAccessRights) share the same no-typed-callback gap, not individually wire-tested

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Global_Identifier | Unsigned32 | app | yes |
| Status_Flags | BACnetStatusFlags | stack | no |
| Reliability | BACnetReliability | app | no |
| Credential_Status | BACnetBinaryPV | app | no |
| Reason_For_Disable | BACnetLIST of BACnetAccessCredentialDisableReason | app | no |
| Authentication_Factors | BACnetARRAY[N] of BACnetCredentialAuthenticationFactor | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Activation_Time | BACnetDateTime | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Expiration_Time | BACnetDateTime | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Credential_Disable | BACnetAccessCredentialDisable | app | no |
| Assigned_Access_Rights | BACnetARRAY[N] of BACnetAssignedAccessRights | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### Access Rights 1 "Cyan" - Enable (true) wire-verified. VERIFIED GAP (TODO.md #2, chipkin/cas-bacnet-stack#2046): Negative_Access_Rules answers Abort(other) on live ReadProperty (required BACnetARRAY of BACnetAccessRule, no typed callback); Positive_Access_Rules shares the same gap

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Global_Identifier | Unsigned32 | app | yes |
| Status_Flags | BACnetStatusFlags | stack | no |
| Reliability | BACnetReliability | app | no |
| Enable | Boolean | app | no |
| Negative_Access_Rules | BACnetARRAY[N] of BACnetAccessRule | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Positive_Access_Rules | BACnetARRAY[N] of BACnetAccessRule | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### Event Log 1 "Beige" - AE-EL-I-B. Added with BACnetStack_AddEventLogObject; Property_List/Status_Flags are stack-generated (accepted here only because the generic table does not credit AddEventLogObject's own storage), Event_State/Enable are the object's stack-held defaults (Enable left off - this example generates no notifications for it to capture, per TODO.md #1). Record_Count=0 wire-verified. VERIFIED DEFECT (TODO.md #3, chipkin/cas-bacnet-stack#2045): adding this object alone causes a continuous, non-fatal 'Failed to set the date/time' internal log flood from Tick 1 - isolated by bisection, independent of every other object in this file

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Event_State | BACnetEventState | stack default, accepted (Generic Enumerated default: `0`) | no |
| Enable | Boolean | stack default, accepted (Generic Boolean default: `false`) | no |

### Schedule 1 "Saffron" - SCHED-I-B (canonical pattern: B-AAC). Writes Cobalt's Present_Value (BACnetDoorValue, datatype 9=Enumerated) at priority 12: unlock weekdays 08:00, lock the rest of the time (Schedule_Default), plus one 2026-12-25 lock exception (inline calendar-Date form - see Cream's note). All Schedule-owned properties are genuinely populated by BACnetStack_AddScheduleObject/AddScheduleWeeklyTimeValue/SetScheduleDefault/etc at start-up; marked accepted only because the generic table does not credit this object-specific host-configuration API

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | Any | stack default, accepted (Stack-generated if commandable (resolves the priority array)) | no |
| Effective_Period | BACnetDateRange | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Schedule_Default | Any | stack | no |
| List_Of_Object_Property_References | BACnetLIST of BACnetDeviceObjectPropertyReference | stack | no |
| Priority_For_Writing | Unsigned(1..16) | stack default, accepted (Generic UnsignedInteger default: `0`) | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Reliability | BACnetReliability | app | no |
| Out_Of_Service | Boolean | app | no |

### Calendar 1 "Cream" - Same inherited gap B-AAC's file header documents against stack issue #963: no customer-facing way to populate Date_List. Saffron's one exception uses the inline calendar-Date form instead of a Cream reference, so this gap does not affect Saffron's own behaviour

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | Boolean | app | no |
| Date_List | BACnetLIST of BACnetCalendarEntry | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### File 1 "Ivory" - DM-BR-B backup/restore payload carrier - stream access, in-memory 4KB buffer via BACnetStack_RegisterCallbackReadFile/WriteFile. File_Type/File_Size/Modification_Date/Archive/Read_Only all have NO stack default per property-profile-reference.md and are all served here. File_Size=0 wire-verified. VERIFIED GAP (TODO.md #5, chipkin/cas-bacnet-stack#2046): a live AtomicReadFile against this object aborted (Abort(other)) during an active backup session - not root-caused in the time available. Backup_And_Restore_State transitions wire-verified: idle -> performing-abackup (ReinitializeDevice startBackup accepted) -> idle (endBackup accepted)

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| File_Type | CharacterString | app | no |
| File_Size | Unsigned | app | no |
| Modification_Date | BACnetDateTime | app | no |
| Archive | Boolean | app | yes |
| Read_Only | Boolean | app | no |
| File_Access_Method | BACnetFileAccessMethod | stack | no |

### Notification Class 1 "Crimson" - Genuinely populated by BACnetStack_AddNotificationClassObject/AddRecipientToNotificationClass at start-up (same convention as B-LSC's Crimson); accepted only because the generic table cannot credit that host-configuration API. Currently unused for alarm routing given TODO.md #1's finding (no algorithm can be armed on Copper) - kept in the object model because AddRecipientToNotificationClass/SetAlarmsAndEventsForObjectEnabled were still exercised and returned success

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Priority | BACnetARRAY[3] of Unsigned | stack default, accepted (Generic UnsignedInteger default: `0`) | no |
| Ack_Required | BACnetEventTransitionBits | stack default, accepted (Generic BitString default: empty bitstring (zero bits - NOT ) | no |
| Recipient_List | BACnetLIST of BACnetDestination | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |

### Network Port 1 "Vermilion" - BACnet/IP; Network_Type and Protocol_Level are set from BACnetStack_AddNetworkPortObject()'s arguments at start-up; Changes_Pending is computed and answered natively by the stack. Reliability has no fault condition this example detects, so it is accepted at the generic default (normal)

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Status_Flags | BACnetStatusFlags | stack | no |
| Reliability | BACnetReliability | stack default, accepted (Generic Enumerated default: `0`) | no |
| Out_Of_Service | Boolean | app | no |
| Network_Type | BACnetNetworkType | app | no |
| Protocol_Level | BACnetProtocolLevel | app | no |
| Changes_Pending | Boolean | app | no |

<!-- OBJECTS-PROPERTIES:END -->

## The BACnet profile example series

<!-- PROFILE-TABLE:BEGIN (generated from cas-bacnet-stack-examples/docs/profile-table.md - do not edit here) -->
The CAS BACnet Stack supports every standardized device profile in ASHRAE 135-2024 Annex L. One example repository per profile shows how. ✅ = the required BIBB (service) is supported by the CAS BACnet Stack; the **Example** column is the state of that profile's tutorial repository.

### Controllers (Annex L.4)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-SS** Smart Sensor | [B-SS-CPP](https://github.com/chipkin/BACnetProfileExample-B-SS-CPP) ✅ | ✅ DS-RP-B · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-SA** Smart Actuator | [B-SA-CPP](https://github.com/chipkin/BACnetProfileExample-B-SA-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-ASC** Application Specific Controller | [B-ASC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ASC-CPP) ✅ · [B-ASC-Node](https://github.com/chipkin/BACnetProfileExample-B-ASC-Node) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B |
| **B-AAC** Advanced Application Controller | [B-AAC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AAC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ AE-N-I-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-CRL-B · ✅ SCHED-I-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B |
| **B-BC** Building Controller | [B-BC-CPP](https://github.com/chipkin/BACnetProfileExample-B-BC-CPP) ✅ | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-RPM-B · ✅ DS-WP-A · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ AE-N-I-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-CRL-B · ✅ SCHED-E-B · ✅ T-VMT-I-B · ✅ T-ATR-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B · ✅ DM-BR-B |

### Life safety controllers (Annex L.5)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-LSC** Life Safety Controller | [B-LSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-LSC-CPP) 🚧 (blocked: [cas-bacnet-stack#2036](https://github.com/chipkin/cas-bacnet-stack/issues/2036)) | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-B · ✅ AE-LS-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B |
| **B-ALSC** Advanced Life Safety Controller | [B-ALSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ALSC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-B · ✅ AE-LS-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-EL-I-B · ✅ SCHED-I-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B |

### Access control controllers (Annex L.6)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-ACC** Access Control Controller | [B-ACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-B · ✅ DS-ACUC-B · ✅ DS-ACSC-B · ☐ AE-AC-B ([cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)) · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-EL-I-B · ✅ SCHED-I-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B · ✅ DM-BR-B |
| **B-AACC** Advanced Access Control Controller | [B-AACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AACC-CPP) ✅ | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-RPM-B · ✅ DS-WP-A · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-A · ✅ DS-COV-B · ✅ DS-ACAD-A · ☐ DS-ACCDI-A · ✅ DS-ACUC-B · ✅ DS-ACSC-B · ☐ AE-AC-B ([cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)) · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-EL-I-B · ✅ SCHED-I-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B · ✅ DM-BR-B |

### Lighting controllers (Annex L.11)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-LD** Lighting Device | [B-LD-CPP](https://github.com/chipkin/BACnetProfileExample-B-LD-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DS-LO-B / DS-BLO-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B |
| **B-LS** Lighting Supervisor | [B-LS-CPP](https://github.com/chipkin/BACnetProfileExample-B-LS-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-WP-B · ✅ DS-WG-E-B · ✅ DS-ALO-A · ✅ SCHED-E-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B |

### Elevator controllers (Annex L.13)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-EM** Elevator Monitor | [B-EM-CPP](https://github.com/chipkin/BACnetProfileExample-B-EM-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-COV-B · ✅ DS-COVM-B · ✅ AE-N-I-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B |
| **B-EC** Elevator Controller | [B-EC-CPP](https://github.com/chipkin/BACnetProfileExample-B-EC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-B · ✅ DS-COVM-B · ✅ AE-N-I-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-RD-B |
| **B-AEC** Advanced Elevator Controller | [B-AEC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AEC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-RPM-B · ✅ DS-WP-B · ✅ DS-WPM-B · ✅ DS-COV-B · ✅ DS-COVM-B · ✅ AE-N-I-B · ✅ AE-ACK-B · ✅ AE-INFO-B · ✅ AE-EL-I-B · ✅ SCHED-I-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B · ✅ DM-OCD-B · ✅ DM-RD-B · ✅ DM-BR-B |

### Authentication and authorization (Annex L.14)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-AS** Authorization Server | [B-AS-CPP](https://github.com/chipkin/BACnetProfileExample-B-AS-CPP) ✅ | ✅ DS-RP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ☐ AA-AS-B ([cas-bacnet-stack#2043](https://github.com/chipkin/cas-bacnet-stack/issues/2043)) |

### Miscellaneous (Annex L.7, combinable with any one family)

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-BBMD** Broadcast Management Device | [B-BBMD-CPP](https://github.com/chipkin/BACnetProfileExample-B-BBMD-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ NM-BBMDC-B |
| **B-ACDC** Access Control Door Controller | [B-ACDC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACDC-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DS-ACAD-B · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-ACCR** Access Control Credential Reader | [B-ACCR-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACCR-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DS-COV-B · ✅ DS-ACCDI-B · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-RTR** Router | [B-RTR-CPP](https://github.com/chipkin/BACnetProfileExample-B-RTR-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-A · ✅ DM-DOB-B · ☐ DM-LM-B · ✅ NM-RC-B |
| **B-GW** Gateway | [B-GW-CPP](https://github.com/chipkin/BACnetProfileExample-B-GW-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ GW-EO-B / GW-VN-B |
| **B-DAP** Device Address Proxy | [B-DAP-CPP](https://github.com/chipkin/BACnetProfileExample-B-DAP-CPP) ✅ | ✅ DS-RP-B · ✅ DS-WP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DAB-B |
| **B-SCHUB** BACnet/SC Hub | [B-SCHUB-CPP](https://github.com/chipkin/BACnetProfileExample-B-SCHUB-CPP) ✅ | ✅ DS-RP-B · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ NM-SCH-B |
| **B-GENERAL** General device (Annex L.8) | *(satisfied by every example above)* | ✅ DS-RP-B · ✅ DM-DDB-B · ✅ DM-DOB-B |

### Operator interfaces and workstations (Annex L.1–L.3, L.9–L.10, L.12) — client-side profiles

| Profile | Example | Required BIBBs (services) |
|---|---|---|
| **B-OD** Operator Display | [B-OD-CPP](https://github.com/chipkin/BACnetProfileExample-B-OD-CPP) ✅ | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-V-A · ✅ DS-M-A · ✅ AE-N-A · ✅ AE-VN-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-OWS** Operator Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-V-A · ✅ DS-M-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-VM-A · ✅ AE-VN-A · ✅ SCHED-VM-A · ✅ T-V-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-MTS-A |
| **B-AWS** Advanced Operator Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-AV-A · ✅ DS-AM-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-AVM-A · ✅ AE-AVN-A · ✅ AE-ELVM-A · ✅ SCHED-AVM-A · ✅ T-AVM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A · ✅ DM-DDA-A · ✅ NM-CC-A · ✅ AR-AVM-A |
| **B-XAWS** Extended Advanced Operator Workstation | planned | ✅ union of B-AWS + B-AACWS + B-ALWS + B-AEWS |
| **B-LSAP** Life Safety Annunciator Panel | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-LSV-A · ✅ AE-N-A · ✅ AE-LS-A · ✅ AE-ACK-A · ✅ AE-LSVN-A |
| **B-LSWS** Life Safety Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-LSV-A · ✅ DS-LSM-A · ✅ AE-N-A · ✅ AE-LS-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-LSVM-A · ✅ AE-LSAVN-A · ✅ AE-ELV-A · ✅ SCHED-VM-A · ✅ T-V-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A |
| **B-ALSWS** Advanced Life Safety Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-LSAV-A · ✅ DS-LSAM-A · ✅ AE-N-A · ✅ AE-LS-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-LSAVM-A · ✅ AE-LSAVN-A · ✅ AE-ELVM-A · ✅ SCHED-AVM-A · ✅ T-AVM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A · ✅ AR-AVM-A |
| **B-ACSD** Access Control Security Display | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-ACV-A · ✅ DS-ACM-A · ✅ AE-N-A · ✅ AE-AC-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-ACAVN-A · ✅ AE-ELV-A · ✅ SCHED-VM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-MTS-A |
| **B-ACWS** Access Control Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-ACAV-A · ✅ DS-ACM-A · ✅ DS-ACUC-A · ✅ AE-N-A · ✅ AE-AC-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-ACVM-A · ✅ AE-ACAVN-A · ✅ AE-ELV-A · ✅ SCHED-VM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A |
| **B-AACWS** Advanced Access Control Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-ACAV-A · ✅ DS-ACAM-A · ✅ DS-ACUC-A · ✅ DS-ACSC-A · ✅ AE-N-A · ✅ AE-AC-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-ACAVM-A · ✅ AE-ACAVN-A · ✅ AE-ELVM-A · ✅ SCHED-AVM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A · ✅ AR-AVM-A |
| **B-LOD** Lighting Operator Display | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-LV-A · ✅ DS-WG-A · ✅ DS-ALO-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-ALWS** Advanced Lighting Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-LAV-A · ✅ DS-LAM-A · ✅ DS-WG-A · ✅ DS-ALO-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-AVM-A · ✅ AE-AVN-A · ✅ AE-ELVM-A · ✅ SCHED-AVM-A · ✅ T-AVM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A |
| **B-LCS** Lighting Control Station | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-LO-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B |
| **B-ALCS** Advanced Lighting Control Station | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-WG-A · ✅ DS-ALO-A · ✅ SCHED-E-B · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B · ✅ DM-DCC-B · ✅ DM-TS-B / DM-UTC-B |
| **B-ED** Elevator Display | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-WP-A · ✅ DS-EV-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-EVN-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-DOB-B |
| **B-EWS** Elevator Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-COVM-A · ✅ DS-EV-A · ✅ DS-EM-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-EVM-A · ✅ AE-EAVN-A · ✅ SCHED-VM-A · ✅ T-V-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A |
| **B-AEWS** Advanced Elevator Workstation | planned | ✅ DS-RP-A · ✅ DS-RP-B · ✅ DS-RPM-A · ✅ DS-WP-A · ✅ DS-WPM-A · ✅ DS-COVM-A · ✅ DS-EAV-A · ✅ DS-EAM-A · ✅ AE-N-A · ✅ AE-ACK-A · ✅ AE-AS-A · ✅ AE-EAVM-A · ✅ AE-EAVN-A · ✅ AE-ELVM-A · ✅ SCHED-AVM-A · ✅ T-AVM-A · ✅ DM-DDB-A · ✅ DM-DDB-B · ✅ DM-ANM-A · ✅ DM-ADM-A · ✅ DM-DOB-B · ✅ DM-DCC-A · ✅ DM-MTS-A · ✅ DM-OCD-A · ✅ DM-RD-A · ✅ DM-BR-A |

Profile definitions: ANSI/ASHRAE 135-2024 Annex L. BIBB definitions: Annex K. Get the stack: <https://store.chipkin.com/services/stacks/bacnet-stack>.
<!-- PROFILE-TABLE:END -->

## Footprint

| OS | Binary size | SHA-256 (first 12) | Start-up time | Stack commit | Link mode | Compiler |
|---|---|---|---|---|---|---|
| Windows (windows-2022) | 3,296,768 bytes | `78ab5f8583ef` | 238 ms | `abd4cee1` | STATIC | Visual Studio 17 2022 |
| Linux (ubuntu-latest) | 59,832 bytes | `52646beb797b` | 128 ms | `abd4cee1` | STATIC | `/usr/bin/c++` |

## References

- [ANSI/ASHRAE 135-2024](https://www.ashrae.org/technical-resources/bookstore/bacnet) (BACnet), Annex L.5.
- CAS BACnet Stack documentation, `submodules/cas-bacnet-stack/docs/`.
- Series bible: `../docs/series-runbook.md`.

## Use this in your own project

This example's `main.cpp`, `CMakeLists.txt`, and `common/` are CC0 (public domain) - see LICENSE.
The CAS BACnet Stack itself is a separate, commercially licensed product.
