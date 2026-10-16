# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - unreleased

### Changed

- **`common/` 3.3.0 -> 3.4.0** and **`wiretest/` 0.5.0 -> 0.6.0**, both copied
  verbatim (`common/` from B-SS-CPP `5d78299`, `wiretest/` from B-LS-CPP). With
  `common/` 3.4.0 a broadcast the stack addresses to this socket's own address
  goes to the subnet broadcast instead; `wiretest/` 0.6.0 only adds helpers.
  `tests/discovery_test.py` now sends its Who-Is a second time, with a visible
  `[RETRY]` line, if no I-Am comes back in 3 s: Who-Is is unconfirmed and the
  example's UDP receive can drop a datagram on Windows. Discovery results are
  unchanged: 23/23 with the 6 known gaps (stack #2150) on Windows
  and Linux, no `[RETRY]`.
- **Labelled a Blocked Example.** The B-ACC Profile Function (authentication
  and authorization for physical access, configured through the Access
  Credential and Access Rights objects and reported as access events;
  ANSI/ASHRAE 135-2024 L.6.2) cannot be performed through the stack's customer
  interface: the authorization-data properties cannot be served
  ([stack #2150](https://github.com/chipkin/cas-bacnet-stack/issues/2150)) and
  no ACCESS_EVENT notification can be generated
  ([stack #2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)). The
  stack owner ruled on 2026-10-15 that neither is promoted for now. README,
  PICS, TODO.md, `main.cpp` and the Device's `Description` say so.
- **BIBB claims corrected** (verified against 135-2024 Annex K): DS-ACUC-B is
  "Access Control User Configuration" (K.1.34) and DS-ACSC-B is "Access Control
  Site Configuration" (K.1.36) - reading and writing the standard properties of
  the access objects - not "Unlock Command" / "Supervisory Command"; both are
  now **not claimed** (#2150). AE-EL-I-B (K.2.21) is a B-AACC BIBB, not a B-ACC
  one, and nothing is logged; no longer claimed. The `main.cpp` header no
  longer says a change-of-state notification substitutes for AE-AC-B (the
  stack rejects that algorithm for an Access Point; no notification is
  produced), and cites Annex L.6.2, not L.5. PICS: SCHED-I-B is "Internal",
  Who-Is is initiated at start-up.
- **Stack Pin moved to `6.x` @ `64b2cfac5` (6.0.23)** from `abd4cee1` (6.0.21).
  Re-checked on the wire: the Event Log log flood (#2045), the Access_Event
  write (#2151) and File_Size (#2152) are fixed. Calendar exports (#963) are
  now on the customer interface; Cream does not use them yet (TODO.md #3).
- **Protocol_Revision 24 -> 30** (stack default at the pin); segmentation is
  now `segmented-both` (stack #2992).
- **`common/` 3.0.0 -> 3.3.0**, copied verbatim from B-SS-CPP `b640a81`.

### Fixed

Found by the new discovery Wire Test at the new pin:

- The Device's `Local_Date` and `Local_Time` answered read-access-denied: the
  stack lists them (the device executes TimeSynchronization and
  UTCTimeSynchronization) but the Date/Time callbacks never served them. They
  now answer wall-clock local time.
- Schedule 1 ("Saffron")'s `Present_Value` answered value-not-initialized,
  because the stack's Schedule engine runs on the Device's `Local_Date` /
  `Local_Time`. With them served, Saffron evaluates and writes Cobalt.
- Credential Data Input 1 ("Flax")'s `Supported_Formats` answered
  unknown-property: the array length (index 0) was never served. It is now,
  as in B-ACCR.

### Added

- **Discovery Wire Test** (`tests/discovery_test.py`, `tests/run_wire_tests.py`,
  from B-SS-CPP `da3e96f`, with this example's 16-object control list): reads
  every property in every object's Property_List. Two additions: Event Log
  `Log_Buffer` must answer read-access-denied (ReadRange only, cl. 12.27.13),
  and the six #2150 properties are reported as `[KNOWN GAP]`s. 23/23 with 6
  known gaps on Windows and Linux. CI runs it on Linux after the smoke test.
- `wiretest/` 0.5.0, vendored verbatim from B-BBMD-CPP `75aedd8`.

## [1.0.2] - unreleased

### Changed

- **Device renamed from the series' colour placeholder "Rainbow" to "Chipkin
  Example B-ACC"** so devices from different examples in the series are
  distinguishable from each other on the same BACnet network - every example
  previously announced the identical Object_Name "Rainbow", which made two
  examples on one subnet indistinguishable by name. Sub-object names (Analog
  Input 1 "Bronze", etc.) are unchanged - only the Device object's name
  changed. `docs/colour-table.md` (series root) updated to match. APP_VERSION
  bumped 1.0.1 -> 1.0.2.

## [Unreleased]

### Fixed

- **`Application_Software_Version` (12) and `Firmware_Revision` (44) were
  hardcoded and stale** - the same bug found and fixed in the sibling
  BACnetProfileExample-B-SCHUB-CPP example via a real device read (BACnet
  Explorer): `Application_Software_Version` reported a hardcoded `"1.0.0"`
  regardless of the actual build, and `Firmware_Revision` was the same stale
  `"1.0.0"` constant - it was never meant to be this example's own version at
  all, it names the underlying platform. Fixed: `Application_Software_Version`
  now reads `APP_VERSION` directly (one source of truth, can't drift from
  `--version`'s own banner again). `Firmware_Revision` is now built at
  runtime from the CAS BACnet Stack's own `BACnetStack_GetAPIMajorVersion()`/
  `GetAPIMinorVersion()`/`GetAPIPatchVersion()`/`GetAPIBuildVersion()` (the
  same 4 calls `common/CASExampleHelper.cpp`'s `PrintVersion()` already uses
  for the startup banner), populated once right after `LoadBACnetFunctions()`
  succeeds. APP_VERSION bumped 1.0.0 -> 1.0.1.

### Changed

- Restructured documentation to match the series' new shape (see
  `BACnetProfileExample-B-SS-CPP`): `README.md` cut down to what this example
  is (profile, device tree, BIBB/object support, build/run/verify, footprint,
  series table), long-form extension/review material moved to a new
  `TUTORIAL.md`, and a new `docs/PICS.md` (ANSI/ASHRAE 135 Annex A shape,
  generated objects-and-properties section) replaces the README's old inline
  conformance table. `docs/objects.json` gained a `Device` entry (previously
  omitted from the generated tables).
- Switched the documented and CI build from a prebuilt **STATIC** stack
  library (`tools/build-stack-static.sh`) to the adapter's default **SOURCE**
  mode: `cmake -B build -S .` / `cmake --build build --config Release` now
  compiles the stack straight into the executable, identically on every
  platform and with no series-root script. `.github/workflows/release.yml`
  dropped the static-library cache/build steps and the matrix `lib:` entries,
  asserts `CAS_BACNET_STACK_LINK=SOURCE`, records `"link_mode": "SOURCE"` in
  `metrics.json`, and now packages `TUTORIAL.md` / `docs/PICS.md` with each
  release. The published Footprint numbers are still from the old STATIC
  build; the next release refreshes them under SOURCE.
- `main.cpp`'s `CHANGE ALL OF THIS BEFORE YOU SHIP` block now carries a
  per-field comment (vendor ID, `DEVICE_NAME` uniqueness warning, model name,
  DCC password) absorbed from the old README's "Before you ship" table.

## [1.0.0] - 2026-09-15

### Added

- Initial **B-ACC (Access Control Controller)** profile example for the CAS BACnet
  Stack in C++. Seeded from B-LSC (Life Safety Controller) with the life-safety
  objects/callbacks removed entirely and the access family added in their place.
  Pinned to the CAS BACnet Stack `6.x` branch at `abd4cee1` (reports 6.0.21),
  linked as a **STATIC** library; vendors `common/` 2.5.0.
- Implements DS-RP-B, DS-RPM-B, DS-WP-B, DS-WPM-B, DS-COV-B, DS-ACUC-B,
  DS-ACSC-B, AE-ACK-B, AE-INFO-B, AE-EL-I-B, SCHED-I-B, DM-BR-B, DM-DDB-A/B,
  DM-DOB-B, DM-DCC-B, DM-TS-B/DM-UTC-B, DM-RD-B.
- Objects: base sensors (Bronze/Emerald/Hot Pink), Access Door 1 "Cobalt"
  (canonical pattern: B-ACDC), Credential Data Input 1 "Flax" (canonical
  pattern: B-ACCR), Access Point 1 "Copper", Access Zone 1 "Ebony", Access
  Credential 1 "Coral", Access Rights 1 "Cyan", Event Log 1 "Beige", Schedule 1
  "Saffron" / Calendar 1 "Cream" (canonical pattern: B-AAC), File 1 "Ivory",
  Notification Class 1 "Crimson", Network Port 1 "Vermilion".
- Verified live on the wire this session (a real BACnet client): Who-Is/I-Am,
  ReadProperty across every object, DS-ACUC-B's WriteProperty unlock command
  (Cobalt's Present_Value/Lock_Status), and DM-BR-B's Backup_And_Restore_State
  transition through `idle -> performing-abackup -> idle` via
  ReinitializeDevice(startBackup)/(endBackup).

### Known gaps (see TODO.md; every one verified against the pinned stack
source and/or the wire, with a filed `chipkin/cas-bacnet-stack` issue)

- AE-AC-B alarm **generation** is not implementable through the customer API
  at this pin: `SetIntrinsicAccessEventAlgorithm`/`SetAccessEventContext` are
  test-tool-only, and the generic `SetIntrinsicChangeOfStateAlgorithmUnsigned`
  substitute rejects `objectType=accessPoint` (confirmed by calling it).
  [chipkin/cas-bacnet-stack#2044](https://github.com/chipkin/cas-bacnet-stack/issues/2044)
- Several REQUIRED constructed-type access-family properties
  (`Authentication_Factors`, `Assigned_Access_Rights`, `Access_Event_Time`,
  `Access_Event_Credential`, `Entry_Points`, `Exit_Points`,
  `Negative_Access_Rules`, `Positive_Access_Rules`) have no customer-facing
  Get callback; confirmed on the wire for two of them.
  [chipkin/cas-bacnet-stack#2046](https://github.com/chipkin/cas-bacnet-stack/issues/2046)
- `BACnetStack_AddEventLogObject` causes a continuous, non-fatal internal log
  flood from the first `Tick()`, isolated by bisection to that call alone.
  [chipkin/cas-bacnet-stack#2045](https://github.com/chipkin/cas-bacnet-stack/issues/2045)
- A live WriteProperty to Copper's `Access_Event` currently answers
  `unknown-object` rather than reaching this file's callback; a live
  `AtomicReadFile` against Ivory during an active backup session aborted;
  neither root-caused in the time available this session.
- Cream's `Date_List` cannot be populated (inherited gap, stack issue #963 -
  same as B-AAC).
