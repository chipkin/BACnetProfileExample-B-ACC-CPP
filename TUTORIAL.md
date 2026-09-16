# Tutorial - extending and reviewing the B-ACC example

[README.md](README.md) says what this example *is*, and which BIBB it cannot
actually deliver. This document is the *how*: how to extend it into your own
access controller, who serves which property, how to review the result for
conformance, and what goes wrong when you get it subtly right.

Read this once before you start changing `main.cpp`. This profile has the
same "silent failure" trap every example in this series has, plus a genuinely
new one of its own - a stack defect that looks exactly like a bug in *your*
code the first time you see it. Both are covered in
[Troubleshooting](#troubleshooting).

- [Extending the example](#extending-the-example)
- [What each object type needs you to serve](#what-each-object-type-needs-you-to-serve)
- [Who serves what: a representative object](#who-serves-what-a-representative-object)
- [Reviewing your device](#reviewing-your-device)
- [Troubleshooting](#troubleshooting)

## Extending the example

The example is intentionally small so it's easy to change, but this profile
has more moving parts than the series' simpler examples - the access family
alone is nine object types plus their Get/Set callback branches.

**Change a sensor's value or name** - edit the constants / callbacks in
`main.cpp` (e.g. the initial value of `g_analogInput1Value`, or the colour
strings in `GetPropertyCharString`).

**Change the device identity before you ship** - vendor ID, vendor name, model
name, description and the DeviceCommunicationControl/ReinitializeDevice
password are all in the `CHANGE ALL OF THIS BEFORE YOU SHIP` block at the top
of `main.cpp`, with a per-field note on each saying what to change it to. That
block is the authoritative checklist; it is in the source rather than here so
it cannot be skipped by someone who only reads the code.

### Add a second Access Door

Read this whole recipe before starting - the last step is the one that is easy
to miss and the one BTL will fail you for.

> **Why skipping a step is SILENT, not loud.** Most of the `GetProperty*`
> callbacks in this file match on **both** object type *and* instance
> (`objectInstance == 1`), so a new instance falls through every one of them.
> The stack errors only for the few properties it refuses to invent -
> `Present_Value` on a non-commandable object, `Relinquish_Default`,
> `File_Size`/`File_Type` with no default, `Local_Date`, `Local_Time`. For
> everything else it **silently substitutes a default**: `Object_Name` reads
> back as `"undefined"`, an enumerated property reads back as its datatype's
> zero value, and a `Boolean` reads back `false`. The object's `Property_List`
> still advertises the property, so a half-added object looks **healthy**, not
> broken - exactly the failure mode the series warns about everywhere else,
> and it applies here with more properties to forget.
>
> Cobalt (Access Door 1) is additionally **commandable**: its `Present_Value`
> is resolved by the stack from the 16-slot `Priority_Array` this file
> maintains (`g_doorIsSet` / `g_doorValue`) plus `Relinquish_Default`. A second
> commandable door needs its own priority-array storage - reusing Cobalt's
> would make the two doors secretly share state.

```cpp
// 1) new instance number + its own priority-array storage (Cobalt's pattern,
//    not shared):
static bool g_door2IsSet[16] = { false };
static double g_door2Value[16] = { 0 };
static double g_door2RelinquishDefault = DOOR_VALUE_LOCK;

// 2) add the object (in main, next to the other BACnetStack_AddObject calls),
//    then make it commandable exactly like Cobalt:
if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 2)) {
    printf("Error: Failed to add Access Door 2.\n");
    return 1;
}
// ... SetPropertyEnabled(Priority_Array), SetPropertyEnabled(Relinquish_Default),
//     SetPropertyWritable(Present_Value) - same three calls as Cobalt's.

// 3) DO NOT SKIP: extend ReadDoorPrioritySlot / CommandDoorWrite /
//    CommandDoorRelinquish (or duplicate them for instance 2) - otherwise a
//    WriteProperty to Access Door 2 silently no-ops instead of commanding it,
//    and every read-back property (Door_Status, Lock_Status, Secured_Status,
//    Reliability, Out_Of_Service, Door_Pulse_Time, ...) needs its own
//    objectInstance == 2 branch in every callback that currently checks
//    objectInstance == 1 for Access Door.
```

Then re-run the README's Verify steps **against Access Door 2**, not just
Cobalt - read every required property and **diff it against Cobalt**. Any
property that comes back `"undefined"`, a zero enumeration, or a value that
doesn't move when you WriteProperty it is a step you missed.

## What each object type needs you to serve

The application must serve every REQUIRED property the stack does not
generate. This differs per type - this is the checklist for the objects this
example adds beyond the series' B-SS baseline (Analog/Binary/Multi-State
Input, Network Port are unchanged from B-SS - see its TUTORIAL if you need
those):

| Object type | You must serve | Plus |
|---|---|---|
| Access Door | `Object_Name`, `Reliability`, `Out_Of_Service`, `Relinquish_Default`, `Door_Pulse_Time`, `Door_Extended_Pulse_Time`, `Door_Open_Too_Long_Time` | `Present_Value`/`Priority_Array` are stack-resolved once commandable; `Door_Status`/`Lock_Status`/`Secured_Status` optional, enabled here |
| Credential Data Input | `Present_Value` (AuthenticationFactor, via `GetPropertyOctetString`), `Object_Name`, `Supported_Formats`, `Update_Time`, `Reliability`, `Out_Of_Service` | Update_Time (not Present_Value) is the COV-subscribable property |
| Access Point | `Object_Name`, `Reliability`, `Out_Of_Service`, `Authentication_Status`, `Active_Authentication_Policy`, `Number_Of_Authentication_Policies`, `Authorization_Mode`, `Access_Event`, `Access_Event_Tag` | `Access_Event_Time`/`Access_Event_Credential` cannot be served - see TODO.md #2 |
| Access Zone | `Object_Name`, `Occupancy_State`, `Reliability`, `Out_Of_Service`, `Global_Identifier` | `Entry_Points`/`Exit_Points` cannot be served - see TODO.md #2 |
| Access Credential | `Object_Name`, `Global_Identifier`, `Reliability`, `Credential_Status`, `Reason_For_Disable`, `Credential_Disable` | `Authentication_Factors`/`Activation_Time`/`Expiration_Time`/`Assigned_Access_Rights` cannot be served - see TODO.md #2 |
| Access Rights | `Object_Name`, `Global_Identifier`, `Reliability`, `Enable` | `Negative_Access_Rules`/`Positive_Access_Rules` cannot be served - see TODO.md #2 |
| Event Log | `Object_Name` | Everything else is stack-generated or a stack-held default once `BACnetStack_AddEventLogObject` runs - see TODO.md #3 for its log-flood side effect |
| Schedule | `Object_Name`, `Reliability`, `Out_Of_Service` | Present_Value/Weekly_Schedule/Schedule_Default/etc. are genuinely populated by `BACnetStack_AddScheduleObject`/`AddScheduleWeeklyTimeValue`/`SetScheduleDefault` at start-up, not a `GetProperty*` callback |
| Calendar | `Object_Name`, `Present_Value` | `Date_List` cannot be populated - see TODO.md #6 |
| File | `Object_Name`, `File_Type`, `File_Size`, `Modification_Date`, `Archive`, `Read_Only` | None of these five have a stack default - forgetting any one is a hard failure, not a silent one |
| Notification Class | `Object_Name` | `Priority`/`Ack_Required`/`Recipient_List` are genuinely populated by `BACnetStack_AddNotificationClassObject`/`AddRecipientToNotificationClass` at start-up |

## Who serves what: a representative object

The single most common question when reading this file is "who answers this
property?" For Access Door 1 (Cobalt), the commandable object at the centre of
DS-ACUC-B, the whole picture:

| Property | Served by | How |
|---|---|---|
| `Object_Identifier` | **stack** | generated from the object you added |
| `Object_Type` | **stack** | generated |
| `Object_Name` | **you** | `GetPropertyCharString` -> `"Cobalt"` |
| `Present_Value` | **stack** | resolved from `Priority_Array` + `Relinquish_Default` once `SetPropertyWritable` makes it commandable |
| `Status_Flags` | **stack** | generated |
| `Event_State` | **stack**, sort of | no intrinsic alarming here, so it reads `normal` only because that is the enumeration's zero value - correct by coincidence |
| `Reliability` | **you** | `GetPropertyEnumerated` -> `RELIABILITY_NO_FAULT_DETECTED` |
| `Out_Of_Service` | **you** | `GetPropertyBool` - matched on object type + instance |
| `Priority_Array` | **stack** | generated once `SetPropertyEnabled` turns it on; `ReadDoorPrioritySlot` mirrors each slot into `GetPropertyBool`/`GetPropertyEnumerated` so a client reading slot N by array index sees this file's own `g_doorIsSet`/`g_doorValue` |
| `Relinquish_Default` | **you** | `GetPropertyEnumerated` -> `g_doorRelinquishDefault` |
| `Door_Status` / `Lock_Status` / `Secured_Status` | **you** | derived in `GetPropertyEnumerated` from whichever priority slot is currently effective |
| `Door_Pulse_Time` / `Door_Extended_Pulse_Time` / `Door_Open_Too_Long_Time` | **you** | `GetPropertyUnsignedInteger`, fixed demo values |

Every object, not just this one, is in [docs/PICS.md](docs/PICS.md).

## Reviewing your device

After you have changed anything, review it against the conformance statement
rather than against "it looked fine in the explorer":

1. Regenerate [docs/PICS.md](docs/PICS.md) after editing `docs/objects.json`
   (see [Keeping the PICS honest](#keeping-the-pics-honest) below). A ⚠ row is
   a required property nothing serves - a real defect, not one of the
   already-documented gaps in TODO.md.
2. Read **every** property listed for **every** object with a BACnet client,
   and compare the value against the PICS. `"undefined"`, a zero enumeration,
   and a stuck `false`/`0` are the shapes a missed callback takes.
3. Diff a new object of a type against the existing one of that type. Anything
   that differs and shouldn't is a callback that matched on instance.
4. **DS-ACUC-B**: WriteProperty Cobalt's `Present_Value` to `unlock`(1) at a
   priority; confirm `Lock_Status`/`Door_Status` follow; relinquish and
   confirm it falls back to `Relinquish_Default`.
5. **DM-BR-B**: `ReinitializeDevice(startBackup)` SimpleACKs;
   `Backup_And_Restore_State` reads `performing-abackup`;
   `ReinitializeDevice(endBackup)` SimpleACKs, state returns to `idle`. Repeat
   for startRestore/AtomicWriteFile/endRestore (not verified working end to
   end in this example - see TODO.md #5).
6. **SCHED-I-B**: confirm Saffron's weekday 08:00 transition (or the
   2026-12-25 exception) actually writes Cobalt at the configured priority
   (12).
7. **DS-COV-B**: SubscribeCOV to Bronze's `Present_Value` or Flax's
   `Update_Time`; change it and confirm a COV notification arrives.
8. Confirm the one BIBB this example does **not** deliver stays disclosed: a
   WriteProperty to Copper's `Access_Event` should still be attempted (per
   TODO.md #4 it currently fails on the wire before reaching this file's
   callback), and no ACCESS_EVENT-typed notification should ever appear,
   because no intrinsic algorithm is armed (TODO.md #1). Do not "fix" this
   quietly by re-enabling an algorithm setter without re-reading TODO.md #1
   first - both setters were confirmed absent/rejected against the pinned
   stack, not skipped out of caution.

### Keeping the PICS honest

`docs/PICS.md` is partly generated. `docs/objects.json` describes each object
and who serves which property; the series tool regenerates the object tables
from it plus the stack's own `docs/property-profile-reference.md` at the
pinned commit:

```bash
python tools/gen-objects-properties.py BACnetProfileExample-B-ACC-CPP            # rewrite
python tools/gen-objects-properties.py BACnetProfileExample-B-ACC-CPP --check    # fail if stale
```

(That tool lives in the example-series repository, not in this one. If you
only have this repository, edit the generated block by hand and keep it
matching the callbacks in `main.cpp`.)

When you add an object or a property to `main.cpp`, update `docs/objects.json`
in the same change and regenerate. The `app` list is what the callbacks serve;
`accepted` is for a required property you deliberately leave to the stack's
default (or, for this profile, a property with **no servable path at all** -
document that distinction in the `note`, quoting the TODO.md item and stack
issue). Anything required, not in `app` and not in `accepted`, comes out as a
⚠ row - that is a defect, not a feature.

## Troubleshooting

| Symptom | Cause / fix |
|---------|-------------|
| On start-up the app prints a wall of red `Error:` lines but the device works | **Expected — this is not your bug**, for the same two reasons as every example in this series (the device hearing its own broadcast I-Am, and a one-time BACnet/SC UUID notice) - see below for the *additional* one specific to this profile. |
| The `Error:` wall never stops - it keeps printing `Failed to set the date` / `Failed to set the time` continuously, for the whole time the device runs | **A real, filed stack defect (TODO.md #3, [cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045)), not a build problem or something you broke.** Adding Event Log 1 ("Beige") via `BACnetStack_AddEventLogObject` triggers this from the very first `BACnetStack_Tick()`, independent of every other object in this file - verified by bisection. The device keeps answering Who-Is/ReadProperty correctly despite it. Do not spend time trying to "fix" your own code for this; if it bothers you locally, comment out the `AddEventLogObject` call (and note you have disabled AE-EL-I-B). |
| Copper's `Access_Event` WriteProperty answers `Error(unknown-object)` | **A real, filed stack defect (TODO.md #4).** `SetPropertyWritable` for this property returns success at start-up, but a live write fails before reaching this file's `SetPropertyEnumerated` callback. Not root-caused. |
| No ACCESS_EVENT notification ever appears, no matter what you write to `Access_Event` | **By design of the substitute, not a bug in this file** (TODO.md #1, [cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)). The dedicated access-event intrinsic algorithm is test-tool-only, and the generic `SetIntrinsicChangeOfStateAlgorithmUnsigned` substitute rejects `objectType=accessPoint`. No intrinsic engine is armed on Copper, so nothing generates a notification. |
| `ReadProperty` of `Access_Credential,1.Authentication_Factors` (or `Access_Rights,1.Negative_Access_Rules`, or similar constructed-type properties) answers `Abort(other)` | **Expected — TODO.md #2**, [cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046). These are `BACnetARRAY`/`BACnetLIST` of a complex constructed type with no typed Get callback and no generic constructed-property callback on the customer surface. |
| `AtomicReadFile` against Ivory aborts during an active backup session | **TODO.md #5** - not root-caused in the time available. `File_Size` itself reads correctly outside a backup session. |
| CMake error: *"CAS BACnet Stack adapter not found under: ..."* | Submodules not initialized. Run `git submodule update --init --recursive` (or pass `-D CAS_STACK_DIR=...`). |
| `CASBACnetStackDLL.h: No such file or directory` | Same - submodules not checked out. |
| Windows: *"No CMAKE_CXX_COMPILER could be found"* | Install Visual Studio with the "Desktop development with C++" workload, then re-run from a fresh terminal. |
| First build seems stuck for minutes | Normal - it's compiling ~600 stack files. Only the first build is slow. |
| App prints *"Failed to bind UDP port 47808"* | Another BACnet program is already using 47808. Stop it, or run with `--port <n>`. |
| Client sends Who-Is but sees no I-Am | Firewall is blocking the port, or the client and device are on different subnets (Who-Is is a broadcast). Allow the port; test on the same subnet first. |
