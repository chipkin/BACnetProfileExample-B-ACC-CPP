# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
- Verified live on the wire this session (`bacpypes3`): Who-Is/I-Am,
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
