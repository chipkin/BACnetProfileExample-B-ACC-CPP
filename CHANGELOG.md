# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

_No changes yet._

## [1.1.0] - 2026-10-27

This is a **Blocked Example**. The CAS BACnet Stack cannot yet perform the full
B-ACC Profile Function (ANSI/ASHRAE 135-2024 L.6.2) through its customer
interface. See the note at the top of the README.

- **Does today:** a discoverable BACnet/IP device with the access object family
  (door, credential reader, access point, zone, credential, rights). Its Schedule
  unlocks the door in a weekday window, and it supports COV,
  DeviceCommunicationControl, time synchronization, ReinitializeDevice and
  backup/restore.
- **Does not do today:** the authorization data (Access Credential, Access Rights
  and Access Point rule and door lists) cannot be served, and no access-event
  notification can be generated. It makes no grant or deny decision. The PICS
  therefore does not claim DS-ACUC-B, DS-ACSC-B, AE-AC-B or AE-EL-I-B.

Built on CAS BACnet Stack 6.x (`69b6249f`, 6.0.23).

### Added

- **Automated self-tests** under `tests/`: a discovery test that reads every
  property of every object, plus time-synchronization and recipient-list tests.
- **TUTORIAL.md** (extending the example, troubleshooting) and **docs/PICS.md**
  (Annex A style).

### Changed

- **Device renamed** from "Rainbow" to "Chipkin Example B-ACC" so examples on one
  network can be told apart. Object names such as "Bronze" are unchanged.
- **Build now compiles the stack source directly into the executable**:
  `cmake -B build -S .` then `cmake --build build --config Release` on every
  platform; the separate static-library step is gone.
- **Protocol_Revision is 30** and segmentation is `segmented-both`.
- **BIBB claims corrected** against Annex K: DS-ACUC-B and DS-ACSC-B (reading and
  writing the standard properties of the access objects) and AE-EL-I-B are no
  longer claimed. SCHED-I-B is listed as internal and Who-Is as sent at start-up.
- **Notification Class 1 `Recipient_List` is writable.** The default entry is the
  single local-broadcast recipient with Process Identifier 0 (cl. 12.21) and
  returns at restart.
- **Access Point 1 `Access_Event_Credential`** now reads as the "no credential"
  reference (cl. 12.31.30) instead of an error.

### Fixed

- **Access Door 1 now accepts pulse-unlock and extended-pulse-unlock** commands
  (cl. 12.26.16, 12.26.17) and relinquishes them after the pulse time. The door
  timings are now 3.0 s, 10.0 s and 3.0 s.
- **ReadRange is enabled.** It was listed in the PICS but rejected, so Event Log
  1's `Log_Buffer` could not be read (cl. 12.27.13).
- **TimeSynchronization and UTCTimeSynchronization set the device clock.**
  Previously they were acknowledged but ignored.
- **SubscribeCOV accepts lifetimes up to 8 hours** (was refused above 1 hour).
- **Analog Input 1 has `COV_Increment`** (1.0), as cl. 12.2 requires for COV.
- **Device `Local_Date` and `Local_Time` are served**, so Schedule 1 now evaluates
  and drives the door.
- **Credential Data Input 1 `Supported_Formats`** is served and lists
  simple-number32 (it previously listed an invalid format).
- **The Device `Description`** no longer reads as an empty string.
- **Application_Software_Version and Firmware_Revision** report the real build
  version and stack version instead of a fixed "1.0.0".
- **Requests no longer go unanswered on Windows** in rare timing cases (shared
  `common/` 3.4.1).
- **PICS** lists the COV notification services the device sends.

## [1.0.0] - 2026-09-15

### Added

- Initial **B-ACC (Access Control Controller)** profile example in C++, built on
  CAS BACnet Stack 6.x (`abd4cee1`, 6.0.21).
- Supports DS-RP-B, DS-RPM-B, DS-WP-B, DS-WPM-B, DS-COV-B, SCHED-I-B, DM-BR-B,
  DM-DDB-A/B, DM-DOB-B, DM-DCC-B, DM-TS-B, DM-UTC-B, DM-RD-B, AE-ACK-B and
  AE-INFO-B.
- Objects: base sensors, Access Door 1 "Cobalt", Credential Data Input 1 "Flax",
  Access Point 1 "Copper", Access Zone 1 "Ebony", Access Credential 1 "Coral",
  Access Rights 1 "Cyan", Event Log 1 "Beige", Schedule 1 "Saffron", Calendar 1
  "Cream", File 1 "Ivory", Notification Class 1 "Crimson", Network Port 1
  "Vermilion".

### Known gaps

- Access-event notifications (AE-AC-B) cannot be generated, and several
  constructed-type access-family properties (for example `Authentication_Factors`
  and the access rules) cannot be served.
- Calendar 1 `Date_List` cannot be populated.
- A WriteProperty to Access Point `Access_Event` and an AtomicReadFile during an
  active backup were not working; the Event Log object floods the console log.
