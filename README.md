# BACnet B-ACC (Access Control Controller) - C++ example

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

> **Versions:** this document describes **example v1.0.0**, built and verified
> against **CAS BACnet Stack 6.0.21** (`6.x` @ `abd4cee1`), at
> **Protocol_Revision 24**, with the vendored `common/` helper at **v2.5.0**.
> Running the example prints all three - if what it prints disagrees with this
> line, trust the program and check `CHANGELOG.md`.

## What is the B-ACC (Access Control Controller) profile?

An **Access Control Controller** (ANSI/ASHRAE 135, Annex L.6) manages door
hardware and credential readers: it presents Access Door, Access Point, Access
Zone, Access Credential and Access Rights objects; reports access events;
schedules door unlock windows; and supports backup/restore of its
configuration. This example implements every BIBB the profile requires except
one - see [What this example does NOT do yet](#what-this-example-does-not-do-yet)
below and [docs/PICS.md](docs/PICS.md) for the precise, per-property picture.

## The device this example creates

| Object | Name | Notes |
|---|---|---|
| Device 389009 | Chipkin Example B-ACC | `--deviceID` overrides |
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
| DS-ACUC-B | Access Control Unlock Command - WriteProperty Cobalt's Present_Value | ✅ |
| DS-ACSC-B | Access Control Supervisory Command - WriteProperty Copper's Authorization_Mode | ✅ |
| AE-AC-B | Report access alarms/events | ☐ **not functional - see below** |
| AE-ACK-B | Accept AcknowledgeAlarm | ✅ |
| AE-INFO-B | Answer GetEventInformation | ✅ |
| AE-EL-I-B | Event Log interface (ReadRange of Beige's Log_Buffer) | ✅ |
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

1. **AE-AC-B alarm generation is not implementable through the customer API at
   this pin.** The dedicated intrinsic access-event algorithm setter is not on
   the customer surface (it was moved to the stack's internal test-tool-only
   interface per a past PR review), and the generic
   `SetIntrinsicChangeOfStateAlgorithmUnsigned` substitute rejects
   `objectType=accessPoint` - confirmed by calling it.
   [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)
2. Several REQUIRED constructed-type properties (`Authentication_Factors`,
   `Assigned_Access_Rights`, `Access_Event_Time`, `Access_Event_Credential`,
   `Entry_Points`, `Exit_Points`, `Negative_Access_Rules`,
   `Positive_Access_Rules`) have no customer-facing Get callback - confirmed on
   the wire for `Authentication_Factors` and `Negative_Access_Rules`.
   [chipkin/cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046)
3. **Adding an Event Log object causes a continuous, non-fatal internal log
   flood** from the first `Tick()` - isolated by bisection to that one call,
   independent of every other object in this file. The device keeps answering
   requests correctly despite it; expect it during your own smoke test, it is
   not a build failure.
   [chipkin/cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045)
4. A live WriteProperty to Copper's `Access_Event` (the credential-read demo
   trigger) currently answers `Error(unknown-object)` rather than reaching
   this file's callback; not root-caused.
5. A live `AtomicReadFile` against Ivory during an active backup session
   aborted; not root-caused in the time available this session. `File_Size`
   reads correctly.
6. Cream's `Date_List` cannot be populated (inherited gap, stack issue #963 -
   same as B-AAC).

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
BACnet Access Control Controller (B-ACC) Example - C++ v1.0.0
CAS BACnet Stack version: 6.0.21.0
Common helper (common/) version: 2.5.0
FYI: Listening for BACnet/IP on UDP port 47821 (Network Port 1).
TX 21 bytes to 192.168.3.255:47821 (broadcast) (Network Port 1)
TX 8 bytes to 192.168.3.255:47821 (broadcast) (Network Port 1)
FYI: Device 389009 ("Chipkin Example B-ACC") ready. Vendor ID 389. Press 'h' for help.
```

The two `TX` lines are the start-up I-Am and Who-Is the device broadcasts to
announce itself; they go to the local subnet broadcast address (computed from
the Network Port's interface). As clients talk to the device you'll see
`RX ... bytes from ...` and `TX ... bytes to ...` lines showing the traffic.

> **A wall of red `Error:` lines at start-up, and a CONTINUOUS stream of them
> for as long as the device runs, is expected and is not your bug.** Verified
> by actually running this build: the continuous part is
> `::CASBACnetStack::BACnetDateTime::operator =() ... Error: Failed to set the
> date` / `... Failed to set the time`, repeating for the life of the process -
> a real, filed stack defect triggered by adding the Event Log object (Beige),
> not something wrong with your build or your machine.
> [chipkin/cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045).
> The rest is one-time start-up noise shared with every example in this series
> (the device hearing its own broadcast I-Am, and a one-time BACnet/SC UUID
> notice). [TUTORIAL.md](TUTORIAL.md#troubleshooting) explains all of it.

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
- **DS-ACUC-B**: `WriteProperty(Cobalt.Present_Value, unlock, priority 8)`
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

For a property-by-property review against the conformance statement, see
[TUTORIAL.md](TUTORIAL.md).


## The BACnet profile example series

<!-- PROFILE-TABLE:BEGIN (generated from cas-bacnet-stack-examples/docs/profile-table.md - do not edit here) -->
The CAS BACnet Stack supports every standardized device profile in ASHRAE 135-2024 Annex L, and there is one example repository per profile. Pick the profile your device claims, then the language you build in. "Ask" means the example hasn't been built yet for that language - [contact Chipkin](https://www.chipkin.com/contact/) if you need one.

### Controllers (Annex L.4)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-SS** Smart Sensor | [B-SS-CPP](https://github.com/chipkin/BACnetProfileExample-B-SS-CPP) | Ask | Ask | Ask | Ask |
| **B-SA** Smart Actuator | [B-SA-CPP](https://github.com/chipkin/BACnetProfileExample-B-SA-CPP) | Ask | Ask | Ask | Ask |
| **B-ASC** Application Specific Controller | [B-ASC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ASC-CPP) | [B-ASC-Node](https://github.com/chipkin/BACnetProfileExample-B-ASC-Node) | Ask | Ask | Ask |
| **B-AAC** Advanced Application Controller | [B-AAC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AAC-CPP) | Ask | Ask | Ask | Ask |
| **B-BC** Building Controller | [B-BC-CPP](https://github.com/chipkin/BACnetProfileExample-B-BC-CPP) | Ask | Ask | Ask | Ask |

### Life safety controllers (Annex L.5)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-LSC** Life Safety Controller | [B-LSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-LSC-CPP) 🚧 | Ask | Ask | Ask | Ask |
| **B-ALSC** Advanced Life Safety Controller | [B-ALSC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ALSC-CPP) | Ask | Ask | Ask | Ask |

### Access control controllers (Annex L.6)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-ACC** Access Control Controller | [B-ACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACC-CPP) | Ask | Ask | Ask | Ask |
| **B-AACC** Advanced Access Control Controller | [B-AACC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AACC-CPP) | Ask | Ask | Ask | Ask |

### Lighting controllers (Annex L.11)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-LD** Lighting Device | [B-LD-CPP](https://github.com/chipkin/BACnetProfileExample-B-LD-CPP) | Ask | Ask | Ask | Ask |
| **B-LS** Lighting Supervisor | [B-LS-CPP](https://github.com/chipkin/BACnetProfileExample-B-LS-CPP) | Ask | Ask | Ask | Ask |

### Elevator controllers (Annex L.13)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-EM** Elevator Monitor | [B-EM-CPP](https://github.com/chipkin/BACnetProfileExample-B-EM-CPP) | Ask | Ask | Ask | Ask |
| **B-EC** Elevator Controller | [B-EC-CPP](https://github.com/chipkin/BACnetProfileExample-B-EC-CPP) | Ask | Ask | Ask | Ask |
| **B-AEC** Advanced Elevator Controller | [B-AEC-CPP](https://github.com/chipkin/BACnetProfileExample-B-AEC-CPP) | Ask | Ask | Ask | Ask |

### Authentication and authorization (Annex L.14)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-AS** Authorization Server | [B-AS-CPP](https://github.com/chipkin/BACnetProfileExample-B-AS-CPP) | Ask | Ask | Ask | Ask |

### Miscellaneous (Annex L.7, combinable with any one family)

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-BBMD** Broadcast Management Device | [B-BBMD-CPP](https://github.com/chipkin/BACnetProfileExample-B-BBMD-CPP) | Ask | Ask | Ask | Ask |
| **B-ACDC** Access Control Door Controller | [B-ACDC-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACDC-CPP) | Ask | Ask | Ask | Ask |
| **B-ACCR** Access Control Credential Reader | [B-ACCR-CPP](https://github.com/chipkin/BACnetProfileExample-B-ACCR-CPP) | Ask | Ask | Ask | Ask |
| **B-RTR** Router | [B-RTR-CPP](https://github.com/chipkin/BACnetProfileExample-B-RTR-CPP) | Ask | Ask | Ask | Ask |
| **B-GW** Gateway | [B-GW-CPP](https://github.com/chipkin/BACnetProfileExample-B-GW-CPP) | Ask | Ask | Ask | Ask |
| **B-DAP** Device Address Proxy | [B-DAP-CPP](https://github.com/chipkin/BACnetProfileExample-B-DAP-CPP) | Ask | Ask | Ask | Ask |
| **B-SCHUB** BACnet/SC Hub | [B-SCHUB-CPP](https://github.com/chipkin/BACnetProfileExample-B-SCHUB-CPP) | Ask | Ask | Ask | Ask |
| **B-GENERAL** General device (Annex L.8) | *(satisfied by every example above)* | — | — | — | — |

### Operator interfaces and workstations (Annex L.1–L.3, L.9–L.10, L.12)

Client-side profiles.

| Profile | C++ | Node.js | C# | Rust | Python |
|---|---|---|---|---|---|
| **B-OD** Operator Display | [B-OD-CPP](https://github.com/chipkin/BACnetProfileExample-B-OD-CPP) | Ask | Ask | Ask | Ask |
| **B-OWS** Operator Workstation | planned | — | — | — | — |
| **B-AWS** Advanced Operator Workstation | planned | — | — | — | — |
| **B-XAWS** Extended Advanced Operator Workstation | planned | — | — | — | — |
| **B-LSAP** Life Safety Annunciator Panel | planned | — | — | — | — |
| **B-LSWS** Life Safety Workstation | planned | — | — | — | — |
| **B-ALSWS** Advanced Life Safety Workstation | planned | — | — | — | — |
| **B-ACSD** Access Control Security Display | planned | — | — | — | — |
| **B-ACWS** Access Control Workstation | planned | — | — | — | — |
| **B-AACWS** Advanced Access Control Workstation | planned | — | — | — | — |
| **B-LOD** Lighting Operator Display | planned | — | — | — | — |
| **B-ALWS** Advanced Lighting Workstation | planned | — | — | — | — |
| **B-LCS** Lighting Control Station | planned | — | — | — | — |
| **B-ALCS** Advanced Lighting Control Station | planned | — | — | — | — |
| **B-ED** Elevator Display | planned | — | — | — | — |
| **B-EWS** Elevator Workstation | planned | — | — | — | — |
| **B-AEWS** Advanced Elevator Workstation | planned | — | — | — | — |

🚧 = in progress. "Ask" = not yet built for that language; contact Chipkin if you need it. Profile definitions: ANSI/ASHRAE 135-2024 Annex L. BIBB definitions: Annex K. Get the stack: <https://store.chipkin.com/services/stacks/bacnet-stack>.
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
