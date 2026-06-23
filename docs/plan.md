# Plan (STUB): B-ACC (Access Control Controller) — C++ example

> **STATUS: STUB.** Seed facts below. Expand from
> [`bacnet-profile-plan-template.md`](../../bacnet-profile-plan-template.md) after the
> sample plans ([B-LD](../../BACnetProfileExample-B-LD-CPP/docs/plan.md),
> [B-BC](../../BACnetProfileExample-B-BC-CPP/docs/plan.md)) are reviewed.

**Profile:** B-ACC · **Family:** Annex L.6 (Access Control Controller) · **Role:** B ·
**Archetype:** Controller · **Difficulty:** 5/5 · **Build wave:** 4

**Thesis:** an access-control controller — door/credential logic across the access
object family, generating **ACCESS_EVENT** alarms, with Event Log, scheduling, and
backup/restore. Canonical source for **F-BACKUP, F-ALARM-AC**.

## Required BIBBs (profiles.md L.6)
`DS-RP-B, DS-RPM-B, DS-WP-B, DS-WPM-B, DS-COV-B, DS-ACUC-B, DS-ACSC-B; AE-AC-B,
AE-ACK-B, AE-INFO-B, AE-EL-I-B; SCHED-I-B; DM-DDB-A,B, DM-DOB-B, DM-DCC-B,
(DM-TS-B or DM-UTC-B), DM-RD-B, DM-BR-B`.

## Services to enable
- RP (1), RPM (14), WP (15), WPM (16), COV (5), DCC (17), TimeSync (24/25),
  ReinitializeDevice (20), GetEventInformation (39), + Event Log + backup/restore.

## Objects (baseline + )
- Access Point 1, Access Zone 1, Access Door 1 (reuse B-ACDC), Access Credential 1,
  Access Rights 1, Credential Data Input 1 (reuse B-ACCR); Notification Class 1,
  Event Log 1, Schedule 1 + Calendar 1 (read-only), File 1 (backup).

## Shared features
- **DEFINE:** F-BACKUP (DM-BR-B — File objects + backup/restore callbacks),
  F-ALARM-AC (`SetIntrinsicAccessEventAlgorithm` + `SetAccessEventContext`).
- **REUSE:** F-ACCESS (B-ACDC/B-ACCR), F-EVENTLOG (B-ALSC), F-COV, F-TIMESYNC,
  F-REINIT, F-SCHED (read-only), F-ALARM, F-DCC, F-OUTPUTS.

## Known stack gaps (target: v6.x.x, actively developed — verify each before building)
- **F-ALARM-AC (ACCESS_EVENT) — pending in v6.** `SetIntrinsicAccessEventAlgorithm`
  / `SetAccessEventContext` are **not yet in the current submodule** (`grep` to
  re-check). Model the access objects + serve their readable properties now; arm
  the ACCESS_EVENT alarm when the API lands. Do **not** mark AE-AC-B ✅ until then.
- **F-SCHED — pending in v6** (schedule execution engine not yet exported) → serve
  read-only Schedule + TODO.
- Recipient-by-address workaround (F-ALARM); confirm F-BACKUP File/backup-restore
  APIs against the current submodule. (profiles.md tracks these as ✅ for a later
  v6 build — but the *pinned* submodule is the source of truth.)

## Notes / open questions
- **Heaviest non-flagship example.** Build after B-ALSC (F-EVENTLOG), B-ACDC/B-ACCR
  (F-ACCESS). Confirm DS-ACUC-B / DS-ACSC-B object support. Keep the access object
  set to the minimum that proves the BIBBs.
