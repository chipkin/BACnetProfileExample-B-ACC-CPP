# AGENTS.md

Guidance for AI coding agents working in this repository. See
<https://agents.md/> for the format. Human contributors should read
[README.md](README.md) first.

## What this project is

A **tutorial** C++ example that implements **as much of** the BACnet **B-ACC
(Access Control Controller)** profile as the standard CAS BACnet Stack's
customer-facing API supports. It is one of a series - one git repo per BACnet
profile - seeded from B-LSC (Life Safety Controller) with the life-safety
objects/callbacks removed entirely and the access family added in their
place. Adds the access family (Access Door, Credential Data Input, Access
Point, Access Zone, Access Credential, Access Rights), an Event Log, a
Schedule/Calendar, a File object, and Backup/Restore. What B-ACC requires but
the stack's customer surface cannot yet do is documented in
[TODO.md](TODO.md) - keep that file honest and current; every entry there is
verified against the pinned stack source and/or the live wire, with a filed
`chipkin/cas-bacnet-stack` issue.

## Layout

This repository is self-contained:

- `main.cpp` - the example device.
- `common/` - the shared helper (vendored).
- `submodules/cas-bacnet-stack/` - the **CAS BACnet Stack** as a git submodule
  (private; compiled from source). After cloning, run
  `git submodule update --init --recursive`.

## Build

This example links the CAS BACnet Stack as a prebuilt **STATIC** library (the
only mode it ships in - see the README's "Link mode" section):

```bash
git submodule update --init --recursive   # once, if not cloned with --recursive
tools/build-stack-static.sh BACnetProfileExample-B-ACC-CPP   # from the series root
cmake -B build -S . -DCAS_BACNET_STACK_LINK=STATIC
cmake --build build --config Release
```

The stack library build takes a few minutes the first time - it compiles the
whole stack (~600 files) once, via the stack's own project files; the example
itself then builds in seconds against that library. Use `-D CAS_STACK_DIR=...`
only if your stack lives outside the bundled submodule.

## Run

```bash
./build/BACnetExampleBACC [--port 47808] [--deviceID 389009]   # Linux/macOS
.\build\Release\BACnetExampleBACC.exe [--port 47808] [--deviceID 389009]   # Windows
```

Interactive keys while running: `h` help, `q` quit, up/down nudge Analog Input 1
(also feeds its COV subscribers).

## Conventions

- Device is named "Rainbow"; objects use the series' colour names; vendor id 389.
- Implement the B-ACC services the stack supports; expose **every required
  property** of each object for Protocol_Revision 24. Anything B-ACC requires
  that is NOT implemented must be listed in [TODO.md](TODO.md) and the README.
- **Every access-family object uses the SAME generic pattern as every other
  object in this series** - `BACnetStack_AddObject` + this file's own Get/Set
  callbacks holding the state - NOT the stack's internal
  `BACnetStackAccessDoor`/`AccessPoint`/etc. engines, whose configuration
  surface (`AddAccessDoorObject`, `AddAccessPointObject`,
  `AddAccessCredentialObject`, `AddAccessRightsObject`, `AddAccessZoneObject`)
  is internal to `BACnetDBDevice` and is not exported through
  `CASBACnetStackDLL.h`. Re-verify this with a fresh
  `grep -n "DllExport.*Access" source/CASBACnetStackDLL.h` before assuming it
  has changed.
- **AE-AC-B has no working alarm-generation path at the pinned commit.**
  `SetIntrinsicAccessEventAlgorithm`/`SetAccessEventContext` are test-tool-only
  (`CASBACnetStackDLL.h`'s own "Sprint 75...PR #169" comment), and the generic
  `SetIntrinsicChangeOfStateAlgorithmUnsigned` substitute rejects
  `objectType=accessPoint` - confirmed by calling it against a running binary.
  Do not re-attempt either without first re-checking whether the stack has
  changed; see TODO.md #1 and the filed issue.
- **`BACnetStack_AddEventLogObject` produces a continuous internal log flood**
  from the first `Tick()`, independent of every other object in this file -
  verified by bisection across 7+ rebuilt configurations this session. It is
  currently believed non-fatal (the device keeps answering requests
  correctly) but was not exhaustively characterised. See TODO.md #3.
- DS-COV-B: `BACnetStack_SetPropertySubscribable` on the property, plus
  `SetCOVSettings`/`SetMaxActiveCOVSubscriptions`; `BACnetStack_UpdateValue`
  also feeds COV subscribers.
- Cobalt (Access Door) is **commandable**: store the 16-slot `Priority_Array`
  + `Relinquish_Default` in the app; let the stack resolve `Present_Value`.
- DeviceCommunicationControl (DM-DCC-B): the stack runs the enable/disable
  state machine; the callback just validates `DCC_PASSWORD` and logs.
- ReinitializeDevice (DM-RD-B **and** DM-BR-B): never restart inside the
  callback - record a deadline for COLDSTART/WARMSTART
  (`CASExampleHelper::RequestRestart`); for the five backup/restore states
  (2..6), just accept (return true) and let the stack's own backup/restore
  engine (driven by the four `RegisterCallbackPrepare/CompleteBackup/Restore`
  callbacks) do the work.
- Match the surrounding code style: `const`-correct parameters, check every
  stack return value, keep `main.cpp` linear and well-commented.
- **Never edit `common/` in this repo alone** - it is a vendored copy shared
  by every example in the series, with its own version (`COMMON_VERSION`) and
  changelog (`common/CHANGELOG.md`).

## How to verify a change

There are no unit tests; verification is behavioural:

1. Build, then run one instance on a clear UDP port.
2. With a BACnet client (e.g. the CAS BACnet Explorer, or `bacpypes3`/`BAC0`),
   send **Who-Is** and confirm **I-Am** from the device instance.
3. **ReadProperty** every required property of every object and confirm the
   values; confirm `Protocol_Revision` is 24 and `Object_List` lists all
   objects. Expect the TODO.md-listed constructed-type properties to Abort.
4. **DS-ACUC-B**: WriteProperty Cobalt's `Present_Value` to `unlock`(1) at a
   priority; confirm `Lock_Status`/`Door_Status` follow; relinquish and
   confirm it falls back to `Relinquish_Default`.
5. **DM-BR-B**: ReinitializeDevice(startBackup) SimpleACKs;
   `Backup_And_Restore_State` reads `performing-abackup`;
   ReinitializeDevice(endBackup) SimpleACKs, state returns to `idle`. Repeat
   for startRestore/AtomicWriteFile/endRestore.
6. **SCHED-I-B**: confirm Saffron's weekday 08:00 transition (or the
   2026-12-25 exception) actually writes Cobalt at the configured priority.
7. **DS-COV-B**: SubscribeCOV to Bronze's `Present_Value` or Flax's
   `Update_Time`; change it and confirm a COV notification arrives.
8. **Device management**: ReinitializeDevice COLDSTART SimpleACKs, then the
   process actually restarts and re-announces with an I-Am; DCC
   `disable-initiation`/`enable` SimpleACK; TimeSynchronization accepted.

Verification is manual (no in-repo test suite ships).

## Releasing

Bump `APP_VERSION` in `main.cpp` and add an entry to [CHANGELOG.md](CHANGELOG.md),
then tag `vX.Y.Z`. The GitHub Actions workflow builds and publishes the release.

## License

The example source code is dedicated to the public domain under
[CC0-1.0](LICENSE). The CAS BACnet Stack is a separate, commercially licensed
product and is not covered by that dedication.
