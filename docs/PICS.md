# BACnet Protocol Implementation Conformance Statement (PICS)

For the **BACnet B-ACC (Access Control Controller) C++ example** -
see [README.md](../README.md).

> This is the PICS **for the example as shipped**. It describes a tutorial
> device announcing itself as a Chipkin demo, not a product. When you turn this
> example into your own device, this document is one of the things you rewrite:
> the vendor, model and version rows all come from the
> `CHANGE ALL OF THIS BEFORE YOU SHIP` block at the top of `main.cpp`. The
> example has **not** been submitted for BTL certification, and several rows
> below are honest, filed gaps in the CAS BACnet Stack's customer-facing API at
> the pinned commit, not omissions in this document - see [TODO.md](../TODO.md).

## 1. Product description

| | |
|---|---|
| **Vendor Name** | Chipkin Automation Systems |
| **Vendor Identifier** | 389 |
| **Product Name** | CAS BACnet Stack Example - B-ACC |
| **Product Model Number** | CAS BACnet Stack Example - B-ACC |
| **Application Software Version** | 1.0.0 |
| **Firmware Revision** | 1.0.0 |
| **BACnet Protocol Version** | 1 |
| **BACnet Protocol Revision** | 24 |

**Product Description:** a BACnet/IP Access Control Controller built on the CAS
BACnet Stack. It presents the access family (Access Door, Credential Data
Input, Access Point, Access Zone, Access Credential, Access Rights), an Event
Log, a Schedule/Calendar pair, a File object for backup/restore, a
Notification Class, three read-only sensor objects inherited from the series'
B-SS baseline, and the required Network Port. It is a tutorial for implementers
of the B-ACC profile, and it is honest in this document and in
[TODO.md](../TODO.md) about the parts of Annex L.5's requirements the CAS
BACnet Stack's customer-facing API cannot yet complete.

## 2. BACnet standardized device profile (Annex L)

**B-ACC - BACnet Access Control Controller** (Annex L.6).

This device claims exactly one profile. Because the B-ACC requirements are a
superset of B-GENERAL's, a conformant B-ACC device also satisfies
**B-GENERAL** (Annex L.8); that is subsumption, not a second claim. **AE-AC-B**
is claimed by the profile but is **not actually functional** in this example -
see §3 and [TODO.md #1](../TODO.md).

## 3. BIBBs supported (Annex K)

| BIBB | Description | Supported |
|---|---|:---:|
| DS-RP-B | Data Sharing - ReadProperty - B | ✅ |
| DS-RPM-B | Data Sharing - ReadPropertyMultiple - B | ✅ |
| DS-WP-B | Data Sharing - WriteProperty - B | ✅ |
| DS-WPM-B | Data Sharing - WritePropertyMultiple - B | ✅ |
| DS-COV-B | Data Sharing - COV - B | ✅ |
| DS-ACUC-B | Data Sharing - Access Control Unlock Command - B | ✅ |
| DS-ACSC-B | Data Sharing - Access Control Supervisory Command - B | ✅ |
| AE-AC-B | Alarm and Event - Access Control - B | ☐ **not functional - see below** |
| AE-ACK-B | Alarm and Event - Acknowledge - B | ✅ |
| AE-INFO-B | Alarm and Event - Information - B | ✅ |
| AE-EL-I-B | Alarm and Event - Event Log - Initiate - B | ✅ |
| SCHED-I-B | Scheduling - Initiate - B | ✅ |
| DM-BR-B | Device Management - Backup and Restore - B | ✅ |
| DM-DDB-A | Device Management - Dynamic Device Binding - A | ✅ |
| DM-DDB-B | Device Management - Dynamic Device Binding - B | ✅ |
| DM-DOB-B | Device Management - Dynamic Object Binding - B | ✅ |
| DM-DCC-B | Device Management - Device Communication Control - B | ✅ |
| DM-TS-B | Device Management - Time Synchronization - B | ✅ |
| DM-UTC-B | Device Management - UTC Time Synchronization - B | ✅ |
| DM-RD-B | Device Management - Reinitialize Device - B | ✅ |

**AE-AC-B is claimed by the profile and its supporting objects/properties/
services are all present** (Access_Event is a real, readable/writable property
on Access Point 1 "Copper", and ConfirmedEventNotification /
UnconfirmedEventNotification are enabled services) **but no event notification
is ever generated**, because the intrinsic algorithm this BIBB depends on is
not on the CAS BACnet Stack's customer-facing API at the pinned commit:
`BACnetStack_SetIntrinsicAccessEventAlgorithm` /
`BACnetStack_SetAccessEventContext` were moved to the stack's test-tool-only
interface, and the generic `SetIntrinsicChangeOfStateAlgorithmUnsigned`
substitute rejects `objectType=accessPoint` (confirmed by calling it against a
running binary). **Filed:**
[chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044).
Treat this row as ☐, not ✅, for BTL purposes; the README's profile table
already reflects this.

No other BIBB is supported. In particular this device does not support any
`-A` variant beyond DM-DDB-A, and does not support trending (T-*).

## 4. Application services supported

| Service | Initiate | Execute |
|---|:---:|:---:|
| ReadProperty | no | **yes** |
| ReadPropertyMultiple | no | **yes** |
| WriteProperty | no | **yes** |
| WritePropertyMultiple | no | **yes** |
| SubscribeCOV | no | **yes** |
| Who-Is | no | **yes** |
| I-Am | **yes** | **yes** |
| Who-Has | no | **yes** |
| I-Have | **yes** | no |
| DeviceCommunicationControl | no | **yes** |
| ReinitializeDevice | no | **yes** |
| TimeSynchronization | no | **yes** |
| UTCTimeSynchronization | no | **yes** |
| AcknowledgeAlarm | no | **yes** |
| GetEventInformation | no | **yes** |
| ConfirmedEventNotification | enabled, **never actually sent** (TODO.md #1) | no |
| UnconfirmedEventNotification | enabled, **never actually sent** (TODO.md #1) | no |
| AtomicReadFile | no | **yes**, but see [TODO.md #5](../TODO.md) (a live read during an active backup session aborted) |
| AtomicWriteFile | no | **yes** |
| ReadRange | no | **yes** (native to the Event Log object once added; not a separately-gated service) |

An unsolicited I-Am is broadcast to the local subnet at start-up, as well as in
response to Who-Is.

## 5. Segmentation capability

Segmentation is **not supported** in either direction
(`Segmentation_Supported` = `no-segmentation`). `Max_APDU_Length_Accepted` is
1476 octets, the BACnet/IP maximum. This is a real constraint for
ReadPropertyMultiple / WritePropertyMultiple against the access family's
larger objects: a request whose reply would exceed one APDU fails rather than
segmenting.

## 6. Standard object types supported

No object is dynamically creatable or deletable at run time.

| Object type | Instance | Object_Name | Writable properties |
|---|:---:|---|---|
| Device | 389009 | Rainbow | - |
| Analog Input | 1 | Bronze | - |
| Binary Input | 1 | Emerald | - |
| Multi-State Input | 1 | Hot Pink | - |
| Access Door | 1 | Cobalt | Present_Value (commandable, DS-ACUC-B) |
| Credential Data Input | 1 | Flax | - |
| Access Point | 1 | Copper | Access_Event, Authorization_Mode (DS-ACSC-B) |
| Access Zone | 1 | Ebony | Global_Identifier |
| Access Credential | 1 | Coral | Global_Identifier |
| Access Rights | 1 | Cyan | Global_Identifier |
| Event Log | 1 | Beige | - |
| Schedule | 1 | Saffron | - |
| Calendar | 1 | Cream | - |
| File | 1 | Ivory | Archive |
| Notification Class | 1 | Crimson | - |
| Network Port | 1 | Vermilion | - |

The device instance is configurable at run time with `--deviceID` (BACnet
requires the device instance to be configurable).

## 7. Data link layer options

**BACnet/IP (Annex J)**, UDP port 47808 (0xBAC0) by default, configurable at
run time with `--port`.

BBMD is not supported, Foreign Device registration is not supported, and
BACnet/SC, MS/TP, Ethernet (Annex H) and PTP are not supported.

## 8. Device address binding

Static device binding is **not supported**. The device answers Who-Is/Who-Has
and does not itself initiate a confirmed request to an unbound device, so it
never needs to bind a peer beyond the recipient address configured for
Notification Class 1 (Crimson), which is set at start-up (local subnet
broadcast by default).

## 9. Networking options

None. The device is not a router, not a BBMD, and does not register as a
foreign device.

## 10. Character sets supported

UTF-8 (ANSI X3.4). Supporting a character set does not imply the device can
handle data in all character sets.

## 11. Objects and properties

<!-- OBJECTS-PROPERTIES:BEGIN (generated by tools/gen-objects-properties.py from docs/objects.json - do not edit here) -->
Every object this example creates, and every REQUIRED property of each (per ANSI/ASHRAE 135-2024 clause 12 and the stack's `docs/property-profile-reference.md`), plus the optional properties the example turns on. **Served by** says who answers a ReadProperty: the **stack** generates it, or the **app** serves it from a `GetProperty*` callback in `main.cpp`. A ⚠ row is a required property the app does not serve and the stack would fill with a default - that is a defect, not a feature.

### Device 389009 "Rainbow" - the device itself; the instance is configurable with --deviceID. The stack rows are device-wide facts only the stack knows - the protocol version and revision it implements, the services and object types it was configured with, the live object list and address-binding table. The accepted rows are the stack's configured defaults for APDU limits, segmentation, system status and database revision; an application that answered them from its own constants could contradict the stack, so this example does not

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| System_Status | BACnetDeviceStatus | stack default, accepted (Generic Enumerated default: `0`) | no |
| Vendor_Name | CharacterString | app | no |
| Vendor_Identifier | Unsigned16 | app | no |
| Model_Name | CharacterString | app | no |
| Firmware_Revision | CharacterString | app | no |
| Application_Software_Version | CharacterString | app | no |
| Description *(optional, enabled)* | CharacterString | app | no |
| Protocol_Version | Unsigned | stack | no |
| Protocol_Revision | Unsigned | stack | no |
| Protocol_Services_Supported | BACnetServicesSupported | stack | no |
| Protocol_Object_Types_Supported | BACnetObjectTypesSupported | stack | no |
| Object_List | BACnetARRAY[N] of BACnetObjectIdentifier | stack | no |
| Max_APDU_Length_Accepted | Unsigned | stack default, accepted (`CAS_BACNET_DEVICE_DEFAULT_MAX_APDU_LENGTH_ACCEPTED`) | no |
| Segmentation_Supported | BACnetSegmentation | stack default, accepted (`BACnetSegmentation::noSegmentation`) | no |
| APDU_Timeout | Unsigned | stack default, accepted (`CAS_BACNET_DEVICE_DEFAULT_APDU_TIMEOUT`) | no |
| Number_Of_APDU_Retries | Unsigned | stack default, accepted (`CAS_BACNET_DEVICE_DEFAULT_NUMBER_OF_APDU_RETRIES`) | no |
| Device_Address_Binding | BACnetLIST of BACnetAddressBinding | stack | no |
| Database_Revision | Unsigned | stack default, accepted (Generic UnsignedInteger default: `0`) | no |
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |
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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

### Calendar 1 "Cream" - Same inherited gap B-AAC's file header documents against stack issue #963: no customer-facing way to populate Date_List. Saffron's one exception uses the inline calendar-Date form instead of a Cream reference, so this gap does not affect Saffron's own behaviour

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Present_Value | Boolean | app | no |
| Date_List | BACnetLIST of BACnetCalendarEntry | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

### Notification Class 1 "Crimson" - Genuinely populated by BACnetStack_AddNotificationClassObject/AddRecipientToNotificationClass at start-up (same convention as B-LSC's Crimson); accepted only because the generic table cannot credit that host-configuration API. Currently unused for alarm routing given TODO.md #1's finding (no algorithm can be armed on Copper) - kept in the object model because AddRecipientToNotificationClass/SetAlarmsAndEventsForObjectEnabled were still exercised and returned success

| Property | Datatype | Served by | Writable |
|---|---|---|:---:|
| Object_Identifier | BACnetObjectIdentifier | stack | no |
| Object_Name | CharacterString | app | no |
| Object_Type | BACnetObjectType | stack | no |
| Priority | BACnetARRAY[3] of Unsigned | stack default, accepted (Generic UnsignedInteger default: `0`) | no |
| Ack_Required | BACnetEventTransitionBits | stack default, accepted (Generic BitString default: empty bitstring (zero bits - NOT ) | no |
| Recipient_List | BACnetLIST of BACnetDestination | stack default, accepted (None known - a read fails with `unknown-property` or an empt) | no |
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

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
| Property_List | BACnetARRAY[N] of BACnetPropertyIdentifier | stack | no |

<!-- OBJECTS-PROPERTIES:END -->

## 12. References

- ANSI/ASHRAE Standard 135-2024, Annex A (PICS template), Annex K (BIBBs),
  Annex L.6 (Access Control Controller).
- [README.md](../README.md) - what this example is and how to build it.
- [TUTORIAL.md](../TUTORIAL.md) - how to extend it, and how to keep this
  document honest when you do.
- [TODO.md](../TODO.md) - every verified gap in the CAS BACnet Stack's
  customer-facing API this example ran into, with filed issues.
