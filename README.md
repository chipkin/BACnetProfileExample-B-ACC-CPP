# BACnet B-ACC (Access Control Controller) - C++ example

> **Blocked Example.** The CAS BACnet Stack cannot yet perform the B-ACC
> Profile Function - authentication and authorization for physical access,
> configured through the device's Access Credential and Access Rights objects
> and reported as access events (ANSI/ASHRAE 135-2024 Annex L.6.2) - through
> its customer interface, so this example stays published but is **blocked**
> on two stack gaps. On 2026-10-15 the stack owner ruled that neither is
> promoted to the customer interface for now:
>
> - **[cas-bacnet-stack#2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150)** -
>   the properties that carry the authorization data cannot be served: an
>   Access Credential's `Authentication_Factors` and `Assigned_Access_Rights`,
>   an Access Rights object's `Positive_Access_Rules` and
>   `Negative_Access_Rules`, and an Access Point's `Access_Doors` (each answers
>   Error unknown-property). So another
>   device cannot read or configure who may pass where, and DS-ACUC-B and
>   DS-ACSC-B cannot be claimed.
> - **[cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)** -
>   no customer-facing algorithm generates an ACCESS_EVENT notification, so
>   AE-AC-B cannot be claimed.
>
> What it does today: it is a discoverable BACnet/IP device with the access
> object family (door, credential reader, access point, zone, credential,
> rights), whose other properties read and write as described below; its
> Schedule unlocks the door in a weekday window; it supports COV,
> DeviceCommunicationControl, time synchronization, ReinitializeDevice and
> backup/restore. It makes **no** grant/deny decision.

A minimal, copy-paste-friendly example showing how to implement **as much of**
the BACnet **B-ACC (Access Control Controller)** device profile as the CAS
BACnet Stack's customer-facing API supports, using the
[CAS BACnet Stack](https://store.chipkin.com/services/stacks/bacnet-stack).
It listens on **BACnet/IP (UDP 47808)**, answers ReadProperty/WriteProperty for
the access family (doors, credentials, access points, zones, rights), and is
discoverable via Who-Is / I-Am. It is seeded from B-LSC (Life Safety
Controller) with the life-safety objects/callbacks removed entirely and the
access family added in their place - see `main.cpp`'s file header for the full
reasoning and every verified stack limitation this profile ran into.

**[Download a prebuilt binary](https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP/releases)**
(Windows and Linux x64) - or build it yourself, see [Build](#build) below.

- **[TUTORIAL.md](TUTORIAL.md)** - how to extend this example and how to review
  it for conformance. Read it when you start turning this into your own device.
- **[docs/PICS.md](docs/PICS.md)** - the Protocol Implementation Conformance
  Statement: every object, every property, and who answers it.

> **Versions:** this document describes **example v1.1.0**, built and verified
> against **CAS BACnet Stack 6.0.23** (`6.x` @ `69b6249f`), at
> **Protocol_Revision 30**, with the vendored `common/` helper at **v3.4.1**.
> Running the example prints all three - if what it prints disagrees with this
> line, trust the program and check `CHANGELOG.md`.

## What is the B-ACC (Access Control Controller) profile?

An **Access Control Controller** (ANSI/ASHRAE 135-2024, Annex L.6.2)
"performs authentication and authorization for physical access. It supports
the modification of its Access Credential and Access Rights objects by another
device." It also lets another device modify its Access Point and Access Zone
objects, generates access events and directs them to recipients, and schedules
values. This example does not perform that function (see the Blocked Example
note above); [What this example does NOT do yet](#what-this-example-does-not-do-yet)
and [docs/PICS.md](docs/PICS.md) give the per-BIBB and per-property picture.

## The device this example creates

| Object | Name | Notes |
|---|---|---|
| Device 389009 | Chipkin Example B-ACC | `--deviceID` overrides |
| Analog Input 1 | Bronze | REAL, degrees Celsius; read-only; COV-subscribable, COV_Increment 1.0 |
| Binary Input 1 | Emerald | active/inactive; read-only |
| Multi-State Input 1 | Hot Pink | state 1..3; read-only |
| Access Door 1 | Cobalt | BACnetDoorValue; commandable (lock/unlock); SCHED-I-B target |
| Credential Data Input 1 | Flax | AuthenticationFactor; Update_Time COV-subscribable |
| Access Point 1 | Copper | Access_Event writable (demo); Authorization_Mode writable, stored, gates nothing |
| Access Zone 1 | Ebony | Occupancy_State |
| Access Credential 1 | Coral | Credential_Status |
| Access Rights 1 | Cyan | Enable; Global_Identifier writable |
| Event Log 1 | Beige | logs nothing: no event notification is ever generated (see TODO.md) |
| Schedule 1 | Saffron | writes Cobalt at priority 12 |
| Calendar 1 | Cream | Date_List empty: added with the generic `AddObject`, not the stack's Calendar engine (see TODO.md) |
| File 1 | Ivory | stream access, backup/restore payload |
| Notification Class 1 | Crimson | routes Copper's alarms (see TODO.md #1) |
| Network Port 1 | Vermilion | the BACnet/IP port (required) |

## What this example supports

### BIBBs (BACnet Interoperability Building Blocks)

| BIBB | Description | Supported |
|------|-------------|:---------:|
| DS-RP-B | Data Sharing - ReadProperty - B | ✅ |
| DS-RPM-B | Data Sharing - ReadPropertyMultiple - B | ✅ |
| DS-WP-B | Data Sharing - WriteProperty - B | ✅ |
| DS-WPM-B | Data Sharing - WritePropertyMultiple - B | ✅ |
| DS-COV-B | Data Sharing - COV - B (Bronze's Present_Value, Flax's Update_Time) | ✅ |
| DS-ACUC-B | Data Sharing - Access Control User Configuration - B (K.1.34: read/write the standard properties of Access Rights, Access User, Access Credential) | ❌ blocked on [#2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150) |
| DS-ACSC-B | Data Sharing - Access Control Site Configuration - B (K.1.36: read/write the standard properties of Access Point, Access Zone, Access Door, Credential Data Input) | ❌ blocked on [#2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150) (Access Point) |
| AE-AC-B | Alarm and Event Management - Access Control - B (K.2.32: generate ACCESS_EVENT notifications) | ❌ blocked on [#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044) |
| AE-ACK-B | Accept AcknowledgeAlarm | ✅ |
| AE-INFO-B | Answer GetEventInformation | ✅ |
| AE-EL-I-B | Event Log - Internal - B (K.2.21: collect event notifications in an internal buffer) | not claimed: not a B-ACC BIBB (the Annex L.6 table lists it for B-AACC only), and Beige logs nothing because no notification is generated |
| SCHED-I-B | Schedule/Calendar initiate (Saffron unlocks Cobalt on a schedule) | ✅ |
| DM-BR-B | Backup and Restore (Ivory carries the payload) | ✅ |
| DM-DDB-A, DM-DDB-B | Who-Is/I-Am (answer + initiate) | ✅ |
| DM-DOB-B | Who-Has/I-Have | ✅ |
| DM-DCC-B | DeviceCommunicationControl | ✅ |
| DM-TS-B / DM-UTC-B | TimeSynchronization / UTCTimeSynchronization | ✅ |
| DM-RD-B | ReinitializeDevice (also drives the backup/restore state machine) | ✅ |

Every required property of every object, and who answers it, is in
[docs/PICS.md](docs/PICS.md).

### What this example does NOT do yet

See `TODO.md` for the full, wire-verified list with filed stack issues. Summary:

1. **No authentication or authorization.** The example makes no grant/deny
   decision; Copper's `Authorization_Mode` is stored but gates nothing.
2. **AE-AC-B: no ACCESS_EVENT notification can be generated through the
   customer API.** The intrinsic access-event algorithm is on the stack's
   test-tool interface only, and the generic change-of-state algorithm
   rejects an Access Point.
   [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)
3. **Five required constructed-type properties cannot be served**:
   `Authentication_Factors`, `Assigned_Access_Rights`, `Positive_Access_Rules`,
   `Negative_Access_Rules` and `Access_Doors` (Error unknown-property), read on
   the wire by the Wire Test at `69b6249f`. (`Access_Event_Credential`, which
   aborted at `64b2cfac5`, now reads as "no credential".)
   [chipkin/cas-bacnet-stack#2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150)
4. Cream (Calendar 1) is added with the generic `BACnetStack_AddObject`, so its
   `Date_List` is empty and Saffron uses an inline exception date. The stack's
   Calendar engine is now on the customer interface (stack #963 is closed:
   `AddCalendarObject`, `AddCalendarDateEntry`,
   `AddScheduleExceptionEventWithCalendarReference`); moving to it is example
   work, not yet done.
5. A full `AtomicReadFile`/`AtomicWriteFile` backup and restore payload
   round-trip has not been re-tested at this pin.

Fixed by the stack since the previous pin and re-checked on the wire at
`64b2cfac5`: the Event Log no longer floods the log
([#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045)),
WriteProperty to Copper's `Access_Event` is accepted and reads back
([#2151](https://github.com/chipkin/cas-bacnet-stack/issues/2151)), and Ivory's
`File_Size` reads as a value
([#2152](https://github.com/chipkin/cas-bacnet-stack/issues/2152)).

## Requires the CAS BACnet Stack (licensed product)

This example **builds against the CAS BACnet Stack, which is a commercial Chipkin
product** - it is not free or open source, and there is no public/trial build.
The stack is referenced here as the **private** git submodule
`submodules/cas-bacnet-stack`; you can only fetch and build it once you have a
CAS BACnet Stack license and access to that repository.

**To get the CAS BACnet Stack (and access to build this example), contact
Chipkin:** <https://store.chipkin.com/services/stacks/bacnet-stack> or
sales@chipkin.com.

You do not need a stack licence to *read* this example, or to run a
[prebuilt release binary](https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP/releases).
The licence is what lets you *build* it - that is the part the stack submodule
gates.

## What's in this repository

This is a **self-contained** project. It ships:

- `main.cpp` - the example device.
- `tests/` - the Wire Tests (see [Wire Tests](#wire-tests)), and `wiretest/`, the
  series' shared Python harness they are written against (vendored, like `common/`).
- `common/` - the shared helper (UDP, callbacks, CLI, keyboard) vendored in.
- `CMakeLists.txt` - the build, the same on Windows, Linux, and macOS.
- `docs/PICS.md` - the conformance statement.
- `docs/objects.json` - source for `docs/PICS.md`'s generated object tables.
- `TODO.md` - every known stack-API gap this example ran into, verified
  against the pinned commit and/or the wire, with filed stack issues.
- `submodules/cas-bacnet-stack/` - the **CAS BACnet Stack as a git submodule**
  (private; requires a license - see above). Its sources are compiled into the
  executable, so there is no library or DLL to build, ship, or install.

## Prerequisites

- A C++17 compiler (MSVC, GCC, or Clang).
- CMake >= 3.15.
- Git (to fetch the stack submodule).

### Windows

- **C++ compiler** - install
  [Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
  (free) and select the **"Desktop development with C++"** workload.
- **CMake** - from <https://cmake.org/download/>, or `winget install Kitware.CMake`.

### Linux / macOS

- Debian/Ubuntu: `sudo apt install build-essential cmake git`
- macOS: `xcode-select --install` and `brew install cmake`

## Build

CMake only, and the same two commands on every platform:

```bash
git clone --recursive https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP.git
cd BACnetProfileExample-B-ACC-CPP

cmake -B build -S .
cmake --build build --config Release
```

Already cloned without `--recursive`? Run `git submodule update --init --recursive`
first - the build needs the stack submodule.

> **The first build takes a few minutes** - it compiles the entire CAS BACnet
> Stack (~600 source files) into the executable. Rebuilds after that are
> incremental and take seconds.

If your CAS BACnet Stack lives somewhere other than the bundled submodule, point
CMake at it: `cmake -B build -S . -D CAS_STACK_DIR=/path/to/cas-bacnet-stack`.

## Run

```bash
# Linux / macOS
./build/BACnetExampleBACC --port 47821

# Windows
.\build\Release\BACnetExampleBACC.exe --port 47821
```

Expected output (`--port 47821` here is just an example port that won't
collide with another instance on the same host; the default is 47808):

```
BACnet Access Control Controller (B-ACC) Example - C++ v1.1.0
CAS BACnet Stack version: 6.0.23.0
Common helper (common/) version: 3.4.1
FYI: Listening for BACnet/IP on UDP port 47821 (Network Port 1).
TX 21 bytes to 192.168.3.255:47821 (broadcast) (Network Port 1) - Unconfirmed: I-Am
TX 8 bytes to 192.168.3.255:47821 (broadcast) (Network Port 1) - Unconfirmed: Who-Is
FYI: Device 389009 ("Chipkin Example B-ACC") ready. Vendor ID 389. Press 'h' for help.
```

Inside the Schedule's weekday window you will also see
`WriteProperty: Access Door 1 (Cobalt) <- unlock(1) @ priority 12` - Saffron
unlocking the door.

The two `TX` lines are the start-up I-Am and Who-Is the device broadcasts to
announce itself; they go to the local subnet broadcast address (computed from
the Network Port's interface). As clients talk to the device you'll see
`RX ... bytes from ...` and `TX ... bytes to ...` lines showing the traffic.

> **Red `Error:` lines at start-up are expected and are not your bug** - one-time
> start-up noise shared with every example in this series (the device hearing
> its own broadcast I-Am, and a one-time BACnet/SC UUID notice).
> [TUTORIAL.md](TUTORIAL.md#troubleshooting) explains it. (The continuous
> "Failed to set the date/time" stream an Event Log caused at stack 6.0.21,
> [#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045), is fixed at
> this pin.)

### Command-line options

| Option | Default | Meaning |
|--------|---------|---------|
| `--port <n>` | `47808` | UDP port to listen on (BACnet/IP). |
| `--deviceID <n>` | `389009` | The device's BACnet instance number (BACnet requires this to be configurable). |
| `--help`, `-h` | - | Show usage and exit. |
| `--version` | - | Print the example, stack, and `common/` helper versions, then exit. |

### Interactive commands

While the example runs, these keys are available:

| Key | Action |
|-----|--------|
| `h` | Show the version information and this command list. |
| `q` | Quit. |
| up arrow | Increase Analog Input 1 (`Bronze`) by 1.1 (also feeds its COV subscribers). |
| down arrow | Decrease Analog Input 1 (`Bronze`) by 1.1 (also feeds its COV subscribers). |

## Verify

Verified live with a real BACnet client ([CAS BACnet Explorer](https://store.chipkin.com/products/tools/cas-bacnet-explorer)), not just a local process check:

- **Who-Is** - I-Am from instance **389009** (vendor **389**).
- **ReadProperty** of every base object plus Cobalt (`Present_Value`/
  `Door_Status`/`Lock_Status`), Flax (`AuthenticationFactor`, `Update_Time`),
  Copper (`Access_Event`, `Authorization_Mode`), Ebony (`Occupancy_State`),
  Coral (`Credential_Status`), the Event Log's `Record_Count`, and Ivory's
  `File_Size`.
- **Door command**: `WriteProperty(Cobalt.Present_Value, unlock, priority 8)`
  accepted; a follow-up read confirmed `Present_Value = unlock` and
  `Lock_Status = unlocked`.
- **DM-BR-B**: `ReinitializeDevice(startBackup)` accepted (SimpleAck);
  `Backup_And_Restore_State` read back `idle -> performing-abackup`;
  `ReinitializeDevice(endBackup)` accepted, state returned to `idle`. This is
  the state-machine transition the profile requires.
- **Not verified / found broken this session** (see TODO.md): a full
  `AtomicReadFile`/`AtomicWriteFile` payload round-trip during backup; the
  STARTRESTORE/`AtomicWriteFile`/ENDRESTORE half of the cycle; a live
  credential-read - ACCESS_EVENT notification (blocked by TODO.md #1); the
  constructed-type properties in TODO.md #2.

That list is from v1.0.0 at stack 6.0.21. At `69b6249f` the Wire Test below
re-read every listed property of every object.

For a property-by-property review against the conformance statement, see
[TUTORIAL.md](TUTORIAL.md).

## Wire Tests

A Python BACnet client drives the running example over BACnet/IP from outside
it, the way a client discovers a device - Who-Is, the Device's `Object_List`,
then for every object its `Property_List` and a ReadProperty of every property
it lists. Every read must come back as a value, except the Event Log's
`Log_Buffer`, which the standard makes ReadRange-only. The five properties stack
#2150 cannot serve are reported as `[KNOWN GAP]` lines and do not fail the run.
Python 3 standard library only:

```bash
python3 tests/run_wire_tests.py --exe build/BACnetExampleBACC          # Linux/macOS
python tests\run_wire_tests.py --exe build\Release\BACnetExampleBACC.exe   # Windows
```

It ends with `ALL WIRE TESTS PASSED` (exit 0). CI runs it on Linux after the
smoke test. [tests/README.md](tests/README.md) says what each check proves. It
does not test the Profile Function: the example cannot perform it (see the
Blocked Example note at the top).


## The BACnet profile example series

<!-- PROFILE-TABLE:BEGIN (generated from cas-bacnet-stack-examples/docs/profile-table.md - do not edit here) -->
There is one example repository per standardized device profile in ASHRAE 135-2024 Annex L. An example marked ⛔ is blocked: the stack cannot yet perform that profile's function through its customer interface, and the example's README says what it does today. Pick the profile your device claims, then the language you build in. "Ask" means the example hasn't been built yet for that language - [contact Chipkin](https://store.chipkin.com/contact-us) if you need one.

### Controllers (Annex L.4)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-SS** Smart Sensor | [B-SS-CPP](https://github.com/chipkin/BACnetProfileExample-B-SS-CPP) | [B-SS-Node](https://github.com/chipkin/BACnetProfileExample-B-SS-Node) | [B-SS-CS](https://github.com/chipkin/BACnetProfileExample-B-SS-CS) | [B-SS-Rust](https://github.com/chipkin/BACnetProfileExample-B-SS-Rust) | [B-SS-Python](https://github.com/chipkin/BACnetProfileExample-B-SS-Python) | [B-SS-Go](https://github.com/chipkin/BACnetProfileExample-B-SS-Go) |
| **B-SA** Smart Actuator | [B-SA-CPP](https://github.com/chipkin/BACnetProfileExample-B-SA-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-ASC** Application Specific Controller | [B-ASC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ASC-CPP) | [B-ASC-Node](https://github.com/chipkin/BACnetProfileExample-B-ASC-Node) | Ask | Ask | Ask | Ask |
| **B-AAC** Advanced Application Controller | [B-AAC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AAC-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-BC** Building Controller | [B-BC-CPP](https://github.com/chipkin/BACnetProfileExample-B-BC-CPP) | Ask | Ask | Ask | Ask | Ask |

### Life safety controllers (Annex L.5)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-LSC** Life Safety Controller | [B-LSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-LSC-CPP) 🚧 | Ask | Ask | Ask | Ask | Ask |
| **B-ALSC** Advanced Life Safety Controller | [B-ALSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ALSC-CPP) | Ask | Ask | Ask | Ask | Ask |

### Access control controllers (Annex L.6)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-ACC** Access Control Controller | [B-ACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP) ⛔ blocked on [stack #2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150), [#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044) | Ask | Ask | Ask | Ask | Ask |
| **B-AACC** Advanced Access Control Controller | [B-AACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AACC-CPP) ⛔ blocked on [stack #2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150), [#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044) | Ask | Ask | Ask | Ask | Ask |

### Lighting controllers (Annex L.11)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-LD** Lighting Device | [B-LD-CPP](https://github.com/chipkin/BACnetProfileExample-B-LD-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-LS** Lighting Supervisor | [B-LS-CPP](https://github.com/chipkin/BACnetProfileExample-B-LS-CPP) | Ask | Ask | Ask | Ask | Ask |

### Elevator controllers (Annex L.13)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-EM** Elevator Monitor | [B-EM-CPP](https://github.com/chipkin/BACnetProfileExample-B-EM-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-EC** Elevator Controller | [B-EC-CPP](https://github.com/chipkin/BACnetProfileExample-B-EC-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-AEC** Advanced Elevator Controller | [B-AEC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AEC-CPP) | Ask | Ask | Ask | Ask | Ask |

### Authentication and authorization (Annex L.14)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-AS** Authorization Server | [B-AS-CPP](https://github.com/chipkin/BACnetProfileExample-B-AS-CPP) ⛔ blocked on [stack #2043](https://github.com/chipkin/cas-bacnet-stack/issues/2043) | Ask | Ask | Ask | Ask | Ask |

### Miscellaneous (Annex L.7, combinable with any one family)

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-BBMD** Broadcast Management Device | [B-BBMD-CPP](https://github.com/chipkin/BACnetProfileExample-B-BBMD-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-ACDC** Access Control Door Controller | [B-ACDC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACDC-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-ACCR** Access Control Credential Reader | [B-ACCR-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACCR-CPP) ⛔ blocked on [stack #3934](https://github.com/chipkin/cas-bacnet-stack/issues/3934) | Ask | Ask | Ask | Ask | Ask |
| **B-RTR** Router | [B-RTR-CPP](https://github.com/chipkin/BACnetProfileExample-B-RTR-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-GW** Gateway | [B-GW-CPP](https://github.com/chipkin/BACnetProfileExample-B-GW-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-DAP** Device Address Proxy | [B-DAP-CPP](https://github.com/chipkin/BACnetProfileExample-B-DAP-CPP) ⛔ blocked on [stack #2032](https://github.com/chipkin/cas-bacnet-stack/issues/2032) | Ask | Ask | Ask | Ask | Ask |
| **B-SCHUB** BACnet/SC Hub | [B-SCHUB-CPP](https://github.com/chipkin/BACnetProfileExample-B-SCHUB-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-GENERAL** General device (Annex L.8) | *(satisfied by every example above)* | — | — | — | — | — |

### Operator interfaces and workstations (Annex L.1–L.3, L.9–L.10, L.12)

Client-side profiles.

| Profile | C++ | Node.js | C# | Rust | Python | Go |
|---|---|---|---|---|---|---|
| **B-OD** Operator Display | [B-OD-CPP](https://github.com/chipkin/BACnetProfileExample-B-OD-CPP) | Ask | Ask | Ask | Ask | Ask |
| **B-OWS** Operator Workstation | planned | — | — | — | — | — |
| **B-AWS** Advanced Operator Workstation | planned | — | — | — | — | — |
| **B-XAWS** Extended Advanced Operator Workstation | planned | — | — | — | — | — |
| **B-LSAP** Life Safety Annunciator Panel | planned | — | — | — | — | — |
| **B-LSWS** Life Safety Workstation | planned | — | — | — | — | — |
| **B-ALSWS** Advanced Life Safety Workstation | planned | — | — | — | — | — |
| **B-ACSD** Access Control Security Display | planned | — | — | — | — | — |
| **B-ACWS** Access Control Workstation | planned | — | — | — | — | — |
| **B-AACWS** Advanced Access Control Workstation | planned | — | — | — | — | — |
| **B-LOD** Lighting Operator Display | planned | — | — | — | — | — |
| **B-ALWS** Advanced Lighting Workstation | planned | — | — | — | — | — |
| **B-LCS** Lighting Control Station | planned | — | — | — | — | — |
| **B-ALCS** Advanced Lighting Control Station | planned | — | — | — | — | — |
| **B-ED** Elevator Display | planned | — | — | — | — | — |
| **B-EWS** Elevator Workstation | planned | — | — | — | — | — |
| **B-AEWS** Advanced Elevator Workstation | planned | — | — | — | — | — |

🚧 = in progress. ⛔ = Blocked Example (blocked on the named stack gap). "Ask" = not yet built for that language; contact Chipkin if you need it. Profile definitions: ANSI/ASHRAE 135-2024 Annex L. BIBB definitions: Annex K. Get the stack: <https://store.chipkin.com/services/stacks/bacnet-stack>.
<!-- PROFILE-TABLE:END -->

## References

- **ANSI/ASHRAE Standard 135** (BACnet) - the protocol standard. Object model
  (Clause 12), services (Clause 15), BACnet/IP (Annex J), device profiles
  (Annex L.6). Purchase / preview via the [ASHRAE store](https://www.ashrae.org/technical-resources/standards-and-guidelines).
- **What is BACnet?** - Chipkin's introduction:
  <https://docs.chipkin.com/protocols/bacnet/>.
- **CAS BACnet Stack** - product page and documentation:
  <https://store.chipkin.com/services/stacks/bacnet-stack>.
- **CAS BACnet Explorer** - client for testing this device:
  <https://store.chipkin.com/products/tools/cas-bacnet-explorer>.
- **Shared helper used by this example** - [`common/README.md`](common/README.md).

See also [TUTORIAL.md](TUTORIAL.md), [docs/PICS.md](docs/PICS.md),
[TODO.md](TODO.md), [CHANGELOG.md](CHANGELOG.md), and [AGENTS.md](AGENTS.md).
