// SPDX-License-Identifier: CC0-1.0
// Public-domain example code (CC0) - see LICENSE. The CAS BACnet Stack itself is
// a separate, commercially licensed product and is not covered by CC0.
// =============================================================================
// BACnet Profile Example - B-ACC (Access Control Controller) - C++
//
// This example implements as much of the BACnet "B-ACC" (Access Control
// Controller) device profile as the CAS BACnet Stack's CUSTOMER-FACING API
// supports today. It is seeded from B-LSC (Life Safety Controller) WITH THE
// LIFE-SAFETY OBJECTS/CALLBACKS REMOVED ENTIRELY (not left as dead code -
// see the "removed, not disabled" note below) and the access family added in
// their place.
//
// A B-ACC (ANSI/ASHRAE 135, Annex L.5) must support:
//
//     DS-RP-B, DS-RPM-B     - ReadProperty + ReadPropertyMultiple,
//     DS-WP-B, DS-WPM-B     - WriteProperty + WritePropertyMultiple,
//     DS-COV-B              - SubscribeCOV,
//     DS-ACUC-B             - Access Control Unlock Command (unlock a door),
//     DS-ACSC-B             - Access Control Supervisory Command (Authorization_Mode,
//                              Lockout, Threat_Level style supervisory control),
//     AE-AC-B                - report access alarms/events,
//     AE-ACK-B, AE-INFO-B    - accept AcknowledgeAlarm, answer GetEventInformation,
//     AE-EL-I-B              - Event Log interface (ReadRange of Log_Buffer),
//     SCHED-I-B              - Schedule/Calendar initiate,
//     DM-BR-B                - Backup and Restore,
//     DM-DDB-A,B, DM-DOB-B   - Who-Is/I-Am (answer + initiate), Who-Has/I-Have,
//     DM-DCC-B               - DeviceCommunicationControl,
//     DM-TS-B / DM-UTC-B     - TimeSynchronization / UTCTimeSynchronization,
//     DM-RD-B                - ReinitializeDevice (also drives DM-BR-B's backup/
//                              restore state transitions - see ReinitializeDevice()).
//
// WHY THE LIFE-SAFETY CODE IS GONE, NOT DISABLED: B-LSC's own file header
// documents a CRITICAL, wire-verified stack defect - BACnetStack_AddObject
// never populates the internal BACnetStackLifeSafetyPoint/Zone engine object,
// so a Life Safety Point/Zone built that way cannot serve almost any property
// (https://github.com/chipkin/cas-bacnet-stack/issues/2036). This profile does
// not need Life Safety Point/Zone objects at all (they are not part of Annex
// L.5), so every line of that object type, its Get/Set callback branches, its
// LifeSafetyOperation responder, and its intrinsic ChangeOfLifeSafety/fault
// algorithm setup was deleted rather than kept as unreachable dead code that
// could carry the same class of bug forward into a profile that does not
// exercise it. grep for "LifeSafety" or "LIFE_SAFETY" in this file: zero hits.
//
// SAME QUESTION ASKED OF THE ACCESS FAMILY, ANSWERED HONESTLY (verified against
// source/CASBACnetStackDLL.h and source/BACnetDBDevice.cpp at the pin, not
// assumed): Access Credential/Point/Rights/Zone each DO have an internal,
// richer engine in BACnetDBDevice (BACnetDBDevice::AddAccessCredentialObject,
// AddAccessPointObject, AddAccessRightsObject, AddAccessZoneObject, and the
// SetAccessXxx/EvaluateAccessRights/AccessZoneEnter family) - but NONE of those
// are exported through CASBACnetStackDLL.h (grepped: zero DllExport hits for
// any of them). So, like Access Door in B-ACDC (this series' canonical F-ACCESS
// copy source) and Credential Data Input in B-ACCR, every access object in
// THIS file is built the generic way every other object in this series is -
// BACnetStack_AddObject() plus this file's own Get/Set callbacks holding the
// object's state - not the stack's internal access engine. This is the SAME
// engineering pattern as B-ACDC/B-ACCR (already released, already proven on
// the wire), so it is not a new risk this file introduces.
//
// A SEPARATE, GENUINELY NEW GAP FOUND WHILE BUILDING THIS EXAMPLE: the intrinsic
// algorithm this profile's AE-AC-B BIBB implies -
// BACnetStack_SetIntrinsicAccessEventAlgorithm / BACnetStack_SetAccessEventContext
// - does NOT exist on the customer surface at this pin. CASBACnetStackDLL.h
// itself says why, right where the exports used to be (paraphrased - the
// comment names the internal-only header and function-name prefix this
// series forbids referencing directly, per Global constraint "customer-facing
// interface only"): moved to the stack's test-tool-only interface per a past
// PR review, because ACCESS_EVENT is exercised from BTL testing, not exposed
// as a customer-facing host API.
// There is also no generic "send this event notification" export on the
// customer surface (only the automatic intrinsic-algorithm path can generate
// one) - see §0.2 of the series runbook. So a true ACCESS_EVENT-typed
// notification cannot be produced by an application built on this stack today.
// See TODO.md #1 (the filed stack issue) for the full trace.
//
// WHAT THIS FILE DOES INSTEAD, AND WHY IT IS AN HONEST SUBSTITUTE, NOT A FAKE:
// Access Point 1 ("Copper") carries the REAL Access_Event property the profile
// requires (cl. 12.31), served by this file exactly like any other enumerated
// property. This file arms BACnetStack_SetIntrinsicChangeOfStateAlgorithmEnum
// (a customer-facing, generic intrinsic algorithm - NOT the missing
// access-specific one) on THAT SAME PROPERTY, so a client sees the correct
// property change to `granted`(1) and gets a real event notification for it -
// generated by the stack's own intrinsic-algorithm engine, not synthesised by
// this file. The one thing that is NOT authentic is the notification's own
// BACnet Event Type: it is reported as changeOfState(1), not accessEvent(9),
// because the algorithm that would produce an accessEvent(9) notification is
// the one confirmed absent above. This is disclosed in the README's support
// table and TODO.md, not hidden.
//
// The device keeps the three read-only B-SS sensor objects and the Network
// Port, and adds the access family:
//
//     Device 389009              "Rainbow"     (instance configurable with --deviceID)
//     Analog Input  1             "Bronze"      (REAL, degrees Celsius; read-only)
//     Binary Input  1             "Emerald"     (active / inactive; read-only)
//     Multi-State Input 1         "Hot Pink"    (state 1..3; read-only)
//     Access Door 1               "Cobalt"      (BACnetDoorValue; WRITABLE, commandable - DS-ACUC-B)
//     Credential Data Input 1     "Flax"        (AuthenticationFactor; read-only, COV-subscribable)
//     Access Point 1              "Copper"      (Access_Event demo target - AE-AC-B, DS-ACSC-B)
//     Access Zone 1                "Ebony"      (Occupancy_State)
//     Access Credential 1          "Coral"      (Credential_Status)
//     Access Rights 1              "Cyan"       (Enable; WRITABLE Global_Identifier)
//     Event Log 1                  "Beige"      (accumulates the Access_Event notification - AE-EL-I-B)
//     Schedule 1 / Calendar 1      "Saffron" / "Cream"   (drives Cobalt's lock schedule - SCHED-I-B)
//     File 1                       "Ivory"      (backup/restore payload carrier - DM-BR-B)
//     Notification Class 1         "Crimson"    (routes Copper's Access_Event notification)
//     Network Port 1                "Vermilion" (the BACnet/IP port - required)
//
// DS-ACUC-B / DS-ACSC-B: Cobalt's Present_Value (a BACnetDoorValue) is
// commandable via the same 16-slot Priority_Array pattern every commandable
// object in this series uses (see B-ACDC's file header for the mechanism) -
// this is the "unlock command" BIBB. DS-ACSC-B (supervisory command) is
// demonstrated by Copper's WRITABLE Authorization_Mode - a management station
// can force `denyAll`/`authorize` the same way a real access panel would be
// put into lockdown.
//
// DS-COV-B: Analog Input 1 (Bronze) and Credential Data Input 1 (Flax)'s
// Update_Time (NOT Present_Value - see B-ACCR's file header for exactly why
// AuthenticationFactor-typed objects subscribe on Update_Time) are
// COV-subscribable.
//
// SCHED-I-B: Saffron writes Cobalt's Present_Value (BACnetDoorValue, datatype
// code 9 = Enumerated) at a low priority whenever a Weekly_Schedule or
// Exception_Schedule entry is active; Schedule_Default applies the rest of
// the time - the natural "unlock the lobby door on weekday mornings" shape for
// an access controller, unlike B-AAC's generic setpoint demo.
//
// DM-BR-B: File 1 (Ivory, stream access) is the backup/restore payload
// carrier. BACnetStack_SetBackupAndRestoreEnabled plus all four
// Prepare/Complete Backup/Restore callbacks are registered; ReinitializeDevice
// accepts the five backup/restore states (startBackup/endBackup/startRestore/
// endRestore/abortRestore, values 2-6) instead of rejecting them the way
// B-LSC's copy of this callback does - see ReinitializeDevice() below.
//
// Interactive keys (handled by the shared helper): h = help, q = quit, up/down
// = nudge Analog Input 1 by +/-1.1. This example follows B-LSC's own
// convention (documented in its file header) of using WriteProperty rather
// than claiming a new interactive key to drive a demo - here, WriteProperty
// to Copper's Access_Event simulates a credential read. Command line:
// --port <n>, --deviceID <n>.
//
// All the UDP/stack plumbing lives in common/CASExampleHelper so this file can
// stay focused on the BACnet logic.
// =============================================================================

#include "CASExampleHelper.h"
#include "CASBACnetStackExampleConstants.h"
#include "CASBACnetStackAdapter.h" // the CAS BACnet Stack C API (BACnetStack_*); call
                                    // LoadBACnetFunctions() before any BACnetStack_* call -
                                    // see the top of main() below.

#include <stdio.h>
#include <string.h>
#include <string> // std::string - g_firmwareRevision, built at runtime; see its own comment below

#if defined(_WIN32)
#include <windows.h> // Sleep()
#else
#include <unistd.h>  // usleep()
#endif

using namespace CASBACnetStackExampleConstants;

// -----------------------------------------------------------------------------
// 1. Example + device configuration
// -----------------------------------------------------------------------------
static const char* APP_NAME = "BACnet Access Control Controller (B-ACC) Example - C++";
static const char* APP_VERSION = "1.0.1";

// The device instance. BACnet requires this to be configurable, so it defaults
// to 389009 (docs/colour-table.md) and can be overridden on the command line
// with --deviceID.
static uint32_t g_deviceInstance = 389009;

// ---- Device identity: CHANGE ALL OF THIS BEFORE YOU SHIP --------------------
// Everything in this block is read by clients and shown to the operator in every
// discovery tool on the network. Left as-is, your product will appear on a real
// site announcing itself as a Chipkin demo. None of it is cosmetic: Object_Name
// must be unique across the BACnet internetwork, and Model_Name / Vendor_Identifier
// are what a building operator uses to identify your device.
//
// This block is the ship checklist. Every constant below has a note saying what
// to change it to; nothing here is safe to leave at its example value.
// -----------------------------------------------------------------------------

// Your BACnet Vendor Identifier. 389 = Chipkin Automation Systems; change this
// to YOUR company's vendor ID before shipping a product. Vendor IDs are assigned
// by ASHRAE - request one (free) at https://bacnet.org/assigned-vendor-ids/.
// Update VENDOR_NAME below to match.
static const uint32_t VENDOR_IDENTIFIER = 389;

// The Device object's Object_Name.
//
// THIS IS THE ONE THAT WILL BITE YOU. Object_Name must be unique across the
// whole BACnet internetwork, and here it is a COMPILE-TIME constant. The device
// instance is runtime-configurable via --deviceID, so it is easy to ship two
// units, configure their instances correctly, and still have BOTH announce
// Object_Name "Rainbow" - a spec violation, and a hard BTL failure. In a real
// product Object_Name must be per-unit configurable too: derive it from a serial
// number, DIP switches, a config file, or add a --deviceName argument.
static const char* DEVICE_NAME = "Rainbow";

// The Device object's Description. Change it to what YOUR device actually is;
// this string describes this tutorial and its BIBB list, which is only correct
// for this exact example.
static const char* DEVICE_DESCRIPTION =
    "Chipkin CAS BACnet Stack example - B-ACC (Access Control Controller) profile. "
    "Demonstrates DS-RP/RPM/WP/WPM-B, DS-COV-B, DS-ACUC-B, DS-ACSC-B, AE-AC-B, "
    "AE-ACK-B, AE-INFO-B, AE-EL-I-B, SCHED-I-B, DM-BR-B, DM-DCC-B, DM-RD-B, "
    "and time synchronisation.";

// Device identity strings (read by clients, and used to populate I-Am).
//   VENDOR_NAME - your company name; it must match VENDOR_IDENTIFIER above.
//   MODEL_NAME  - your model designation. This is what a building operator reads
//                 to identify your device in a discovery tool.
static const char* VENDOR_NAME = "Chipkin Automation Systems";
static const char* MODEL_NAME = "CAS BACnet Stack Example - B-ACC";

// DeviceCommunicationControl / ReinitializeDevice password. "" = no password
// required, which is fine for this tutorial but not for a shipped product:
// anyone on the network can disable communication or trigger a restart /
// backup / restore. Pick a real password (and store it somewhere better than a
// compile-time string literal - a config file or secure storage) before you ship.
static const char* DCC_PASSWORD = "";

// Application_Software_Version (12) is just APP_VERSION - one source of
// truth, so it can never drift from what --version/the startup banner
// prints (it did drift: this used to be a separate hardcoded "1.0.0"
// constant nobody updated across several patch releases - found via a real
// device read, not code review, by someone actually testing the built
// device's Device object properties).
//
// Firmware_Revision (44) is meant to name the underlying platform/stack,
// not this example's own version - built at runtime from the CAS BACnet
// Stack's own BACnetStack_GetAPIMajorVersion()/etc. (the same 4 calls
// common/CASExampleHelper.cpp's PrintVersion() already uses for the
// startup banner's "CAS BACnet Stack version: X.Y.Z.W" line), so it can
// never go stale either - see g_firmwareRevision below, populated once
// right after LoadBACnetFunctions() succeeds (those functions are what
// the version getters themselves are, so they must be loaded first).
static std::string g_firmwareRevision;

// The sensor objects (all instance 1) and their colour names.
static const uint32_t ANALOG_INPUT_INSTANCE = 1;       // "Bronze"
static const uint32_t BINARY_INPUT_INSTANCE = 1;       // "Emerald"
static const uint32_t MULTI_STATE_INPUT_INSTANCE = 1;  // "Hot Pink"
static const uint32_t MULTI_STATE_INPUT_NUMBER_OF_STATES = 3;

// The Network Port object.
static const uint32_t NETWORK_PORT_INSTANCE = 1;       // "Vermilion"
static const uint32_t MAX_APDU_LENGTH = 1476;

static uint8_t g_ipAddress[4] = { 0, 0, 0, 0 };
static uint8_t g_ipSubnetMask[4] = { 0, 0, 0, 0 };
static uint8_t g_ipDefaultGateway[4] = { 0, 0, 0, 0 };
static uint16_t g_bacnetIpUdpPort = 47808;

// Analog Input 1's live present value (degrees Celsius).
static float g_analogInput1Value = 21.5f;

// -----------------------------------------------------------------------------
// Access-Control constants used only by this example (not in common/). Full
// lists: submodules/cas-bacnet-stack/source/BACnetObjectType.h,
// BACnetPropertyIdentifier.h, BACnetServicesSupported.h - every number here was
// grepped against the pin, not remembered.
// -----------------------------------------------------------------------------
static const uint16_t OBJECT_TYPE_ACCESS_DOOR = 30;
static const uint16_t OBJECT_TYPE_ACCESS_CREDENTIAL = 32;
static const uint16_t OBJECT_TYPE_ACCESS_POINT = 33;
static const uint16_t OBJECT_TYPE_ACCESS_RIGHTS = 34;
static const uint16_t OBJECT_TYPE_ACCESS_ZONE = 36;
static const uint16_t OBJECT_TYPE_CREDENTIAL_DATA_INPUT = 37;
static const uint16_t OBJECT_TYPE_EVENT_LOG = 25;
static const uint16_t OBJECT_TYPE_FILE = 10;
static const uint16_t OBJECT_TYPE_SCHEDULE = 17;
static const uint16_t OBJECT_TYPE_CALENDAR = 6;

static const uint32_t PROPERTY_IDENTIFIER_RELIABILITY = 103;
static const uint32_t PROPERTY_IDENTIFIER_DOOR_EXTENDED_PULSE_TIME = 227;
static const uint32_t PROPERTY_IDENTIFIER_DOOR_OPEN_TOO_LONG_TIME = 229;
static const uint32_t PROPERTY_IDENTIFIER_DOOR_PULSE_TIME = 230;
static const uint32_t PROPERTY_IDENTIFIER_DOOR_STATUS = 231;
static const uint32_t PROPERTY_IDENTIFIER_LOCK_STATUS = 233;
static const uint32_t PROPERTY_IDENTIFIER_SECURED_STATUS = 235;
static const uint32_t PROPERTY_IDENTIFIER_ACCESS_EVENT = 247;
static const uint32_t PROPERTY_IDENTIFIER_ACCESS_EVENT_CREDENTIAL = 249;
static const uint32_t PROPERTY_IDENTIFIER_ACCESS_EVENT_TAG = 322;
static const uint32_t PROPERTY_IDENTIFIER_ACCESS_EVENT_TIME = 250;
static const uint32_t PROPERTY_IDENTIFIER_ACTIVATION_TIME = 254;
static const uint32_t PROPERTY_IDENTIFIER_ACTIVE_AUTHENTICATION_POLICY = 255;
static const uint32_t PROPERTY_IDENTIFIER_ASSIGNED_ACCESS_RIGHTS = 256;
static const uint32_t PROPERTY_IDENTIFIER_AUTHENTICATION_FACTORS = 257;
static const uint32_t PROPERTY_IDENTIFIER_AUTHENTICATION_STATUS = 260;
static const uint32_t PROPERTY_IDENTIFIER_AUTHORIZATION_MODE = 261;
static const uint32_t PROPERTY_IDENTIFIER_CREDENTIAL_DISABLE = 263;
static const uint32_t PROPERTY_IDENTIFIER_CREDENTIAL_STATUS = 264;
static const uint32_t PROPERTY_IDENTIFIER_ENABLE = 133;
static const uint32_t PROPERTY_IDENTIFIER_ENTRY_POINTS = 268;
static const uint32_t PROPERTY_IDENTIFIER_EXIT_POINTS = 269;
static const uint32_t PROPERTY_IDENTIFIER_EXPIRATION_TIME = 270;
static const uint32_t PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER = 323;
static const uint32_t PROPERTY_IDENTIFIER_NEGATIVE_ACCESS_RULES = 288;
static const uint32_t PROPERTY_IDENTIFIER_NUMBER_OF_AUTHENTICATION_POLICIES = 289;
static const uint32_t PROPERTY_IDENTIFIER_OCCUPANCY_STATE = 296;
static const uint32_t PROPERTY_IDENTIFIER_POSITIVE_ACCESS_RULES = 302;
static const uint32_t PROPERTY_IDENTIFIER_REASON_FOR_DISABLE = 303;
static const uint32_t PROPERTY_IDENTIFIER_UPDATE_TIME = 189;
static const uint32_t PROPERTY_IDENTIFIER_SUPPORTED_FORMATS = 304;
static const uint32_t PROPERTY_IDENTIFIER_FILE_TYPE = 43;
static const uint32_t PROPERTY_IDENTIFIER_FILE_SIZE = 42;
static const uint32_t PROPERTY_IDENTIFIER_MODIFICATION_DATE = 71;
static const uint32_t PROPERTY_IDENTIFIER_ARCHIVE = 13;
static const uint32_t PROPERTY_IDENTIFIER_READ_ONLY = 99;

// -- BACnetDoorValue (Cobalt's Present_Value) --------------------------------
static const uint32_t DOOR_VALUE_LOCK = 0;
static const uint32_t DOOR_VALUE_UNLOCK = 1;

// -- BACnetDoorStatus / BACnetLockStatus / BACnetDoorSecuredStatus (optional,
//    enabled below so this example demonstrates the full Access Door table). --
static const uint32_t DOOR_STATUS_CLOSED = 0;
static const uint32_t LOCK_STATUS_LOCKED = 0;
static const uint32_t LOCK_STATUS_UNLOCKED = 1;
static const uint32_t DOOR_SECURED_STATUS_SECURED = 0;
static const uint32_t DOOR_SECURED_STATUS_UNSECURED = 1;

// -- BACnetAccessEvent (Copper's Access_Event - the AE-AC-B demo trigger) ----
static const uint32_t ACCESS_EVENT_NONE = 0;
static const uint32_t ACCESS_EVENT_GRANTED = 1;
static const uint32_t ACCESS_EVENT_DENIED_UNKNOWN_CREDENTIAL = 129;

// -- BACnetAuthenticationStatus / BACnetAuthorizationMode (Copper) ----------
static const uint32_t AUTHENTICATION_STATUS_READY = 1;
static const uint32_t AUTHORIZATION_MODE_AUTHORIZE = 0;
static const uint32_t AUTHORIZATION_MODE_DENY_ALL = 2;

// -- BACnetAccessZoneOccupancyState (Ebony) ----------------------------------
static const uint32_t OCCUPANCY_STATE_NORMAL = 0;

// -- BACnetBinaryPV, reused for Access Credential's Credential_Status (Coral) -
static const uint32_t CREDENTIAL_STATUS_ACTIVE = 1;

static const uint32_t RELIABILITY_NO_FAULT_DETECTED = 0;
static const uint32_t SERVICE_SUBSCRIBE_COV = 5;
static const uint32_t SERVICE_ATOMIC_READ_FILE = 6;
static const uint32_t SERVICE_ATOMIC_WRITE_FILE = 7;

// -- File 1 "Ivory" (DM-BR-B's backup/restore payload carrier) --------------
static const uint32_t FILE_INSTANCE = 1;
static const uint32_t FILE_ACCESS_METHOD_STREAM = 1;
static const uint32_t FILE_MAX_SIZE = 4096;
static uint8_t g_fileData[FILE_MAX_SIZE];
static uint32_t g_fileDataLength = 0;
static bool g_fileArchive = false;

// -- BACnetReinitializedStateOfDevice backup/restore states (135-2024
//    cl. 12.11.39) - not in common/, which only knows COLDSTART/WARMSTART. --
static const uint32_t REINITIALIZE_STATE_STARTBACKUP = 2;
static const uint32_t REINITIALIZE_STATE_ENDBACKUP = 3;
static const uint32_t REINITIALIZE_STATE_STARTRESTORE = 4;
static const uint32_t REINITIALIZE_STATE_ENDRESTORE = 5;
static const uint32_t REINITIALIZE_STATE_ABORTRESTORE = 6;

// -- Schedule 1 "Saffron" / Calendar 1 "Cream" (SCHED-I-B) -------------------
static const uint32_t SCHEDULE_INSTANCE = 1;
static const uint32_t CALENDAR_INSTANCE = 1;
static const uint8_t SCHEDULE_WRITE_PRIORITY = 12;

// -- Event Log 1 "Beige" (AE-EL-I-B) -----------------------------------------
static const uint32_t EVENT_LOG_INSTANCE = 1;
static const uint32_t EVENT_LOG_MAX_BUFFER_SIZE = 64;

static const uint32_t NOTIFICATION_CLASS_INSTANCE = 1;     // "Crimson"
static const uint8_t NC_PRIORITY_TO_OFFNORMAL = 100;
static const uint8_t NC_PRIORITY_TO_FAULT = 50;
static const uint8_t NC_PRIORITY_TO_NORMAL = 200;

// Recipient addressing - see B-LSC's file header for the broadcast-vs-known-
// client discussion this is copied from.
static const uint32_t RECIPIENT_PROCESS_IDENTIFIER = 1;
static const bool RECIPIENT_USE_BROADCAST = true;
static uint8_t RECIPIENT_IP[4] = { 0, 0, 0, 0 };

// COV settings (DS-COV-B).
static const uint32_t COV_MAX_ACTIVE_SUBSCRIPTIONS = 32;
static const uint32_t COV_MAX_SUPPORTED_LIFETIME_SECONDS = 3600;

// A WriteProperty to a commandable Present_Value carries a priority 1..16.
static uint8_t EffectivePriority(uint8_t priority) {
    return (priority >= 1 && priority <= 16) ? priority : 16;
}

// -----------------------------------------------------------------------------
// Cobalt (Access Door 1) - the commandable object. Same 16-slot Priority_Array
// mechanism every commandable object in this series uses (see B-ACDC/B-LSC's
// file headers for the full mechanism explanation) - not repeated here.
// -----------------------------------------------------------------------------
static bool g_doorIsSet[16] = { false };
static double g_doorValue[16] = { 0 };
static double g_doorRelinquishDefault = DOOR_VALUE_LOCK;

static void CommandDoorWrite(uint8_t priority, double value) {
    const uint8_t p = EffectivePriority(priority);
    g_doorIsSet[p - 1] = true;
    g_doorValue[p - 1] = value;
}
static void CommandDoorRelinquish(uint8_t priority) {
    const uint8_t p = EffectivePriority(priority);
    g_doorIsSet[p - 1] = false;
}
static bool ReadDoorPrioritySlot(uint32_t propertyIdentifier, bool useArrayIndex,
                                 uint32_t propertyArrayIndex, bool* slotIsSet, double* slotValue) {
    if (propertyIdentifier != PROPERTY_IDENTIFIER_PRIORITY_ARRAY || !useArrayIndex ||
        propertyArrayIndex < 1 || propertyArrayIndex > 16) {
        return false;
    }
    *slotIsSet = g_doorIsSet[propertyArrayIndex - 1];
    *slotValue = g_doorValue[propertyArrayIndex - 1];
    return true;
}

// Copper (Access Point 1)'s live state.
static uint32_t g_accessEventValue = ACCESS_EVENT_NONE;
static uint32_t g_authorizationMode = AUTHORIZATION_MODE_AUTHORIZE;

// Coral (Access Credential 1) / Cyan (Access Rights 1)'s writable Global_Identifier.
static uint32_t g_credentialGlobalIdentifier = 0;
static uint32_t g_rightsGlobalIdentifier = 0;
static uint32_t g_zoneGlobalIdentifier = 0;

// -----------------------------------------------------------------------------
// 2. Property "get" callbacks - see B-LSC's file header ("ADDING AN OBJECT?
// READ THIS FIRST") for the errorCode out-parameter contract and the pitfalls
// of a half-added object; the same rules apply here and are not repeated.
// -----------------------------------------------------------------------------

bool GetPropertyReal(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     float* value, const bool useArrayIndex,
                     const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)errorCode;
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_ANALOG_INPUT && objectInstance == ANALOG_INPUT_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
        *value = g_analogInput1Value;
        return true;
    }
    return false;
}

bool GetPropertyEnumerated(const uint32_t deviceInstance, const uint16_t objectType,
                           const uint32_t objectInstance, const uint32_t propertyIdentifier,
                           uint32_t* value, const bool useArrayIndex,
                           const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)errorCode;
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }

    // Reliability (required on every access object below, and on Schedule).
    if (propertyIdentifier == PROPERTY_IDENTIFIER_RELIABILITY &&
        (objectType == OBJECT_TYPE_ACCESS_DOOR || objectType == OBJECT_TYPE_ACCESS_POINT ||
         objectType == OBJECT_TYPE_ACCESS_ZONE || objectType == OBJECT_TYPE_ACCESS_CREDENTIAL ||
         objectType == OBJECT_TYPE_ACCESS_RIGHTS || objectType == OBJECT_TYPE_SCHEDULE)) {
        *value = RELIABILITY_NO_FAULT_DETECTED;
        return true;
    }

    if (objectType == OBJECT_TYPE_BINARY_INPUT && objectInstance == BINARY_INPUT_INSTANCE) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
            *value = 1; // active
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_POLARITY) {
            *value = POLARITY_NORMAL;
            return true;
        }
    }
    if (propertyIdentifier == PROPERTY_IDENTIFIER_UNITS &&
        objectType == OBJECT_TYPE_ANALOG_INPUT && objectInstance == ANALOG_INPUT_INSTANCE) {
        *value = ENGINEERING_UNITS_DEGREES_CELSIUS;
        return true;
    }
    if (objectType == OBJECT_TYPE_NETWORK_PORT && objectInstance == NETWORK_PORT_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_BACNET_IP_MODE) {
        *value = BACNET_IP_MODE_NORMAL;
        return true;
    }

    // -- Cobalt (Access Door 1) --------------------------------------------
    if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1) {
        bool slotIsSet = false;
        double slotValue = 0.0;
        if (ReadDoorPrioritySlot(propertyIdentifier, useArrayIndex, propertyArrayIndex,
                                 &slotIsSet, &slotValue)) {
            if (!slotIsSet) {
                return false;
            }
            *value = (uint32_t)slotValue;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_RELINQUISH_DEFAULT) {
            *value = (uint32_t)g_doorRelinquishDefault;
            return true;
        }
        // Effective Present_Value when no priority slot is set (relinquished):
        // the stack normally resolves this itself from Priority_Array +
        // Relinquish_Default, but only once every slot has answered false via
        // the boolean getter above AND this Enumerated getter has been asked
        // directly. Mirror that here so Present_Value never reads
        // value-not-initialized.
        if (propertyIdentifier == PROPERTY_IDENTIFIER_DOOR_STATUS) {
            *value = DOOR_STATUS_CLOSED;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_LOCK_STATUS) {
            // Reflects whichever DoorValue is currently effective.
            bool anySet = false;
            double effective = g_doorRelinquishDefault;
            for (int i = 0; i < 16; ++i) {
                if (g_doorIsSet[i]) { effective = g_doorValue[i]; anySet = true; break; }
            }
            (void)anySet;
            *value = ((uint32_t)effective == DOOR_VALUE_UNLOCK) ? LOCK_STATUS_UNLOCKED : LOCK_STATUS_LOCKED;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_SECURED_STATUS) {
            bool anySet = false;
            double effective = g_doorRelinquishDefault;
            for (int i = 0; i < 16; ++i) {
                if (g_doorIsSet[i]) { effective = g_doorValue[i]; anySet = true; break; }
            }
            (void)anySet;
            *value = ((uint32_t)effective == DOOR_VALUE_UNLOCK) ? DOOR_SECURED_STATUS_UNSECURED : DOOR_SECURED_STATUS_SECURED;
            return true;
        }
    }

    // -- Copper (Access Point 1) - the AE-AC-B / DS-ACSC-B demo object -----
    if (objectType == OBJECT_TYPE_ACCESS_POINT && objectInstance == 1) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_ACCESS_EVENT) {
            *value = g_accessEventValue;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_AUTHENTICATION_STATUS) {
            *value = AUTHENTICATION_STATUS_READY;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_AUTHORIZATION_MODE) {
            *value = g_authorizationMode;
            return true;
        }
    }

    // -- Ebony (Access Zone 1) ----------------------------------------------
    if (objectType == OBJECT_TYPE_ACCESS_ZONE && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_OCCUPANCY_STATE) {
        *value = OCCUPANCY_STATE_NORMAL;
        return true;
    }

    // -- Coral (Access Credential 1) -----------------------------------------
    if (objectType == OBJECT_TYPE_ACCESS_CREDENTIAL && objectInstance == 1) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_CREDENTIAL_STATUS) {
            *value = CREDENTIAL_STATUS_ACTIVE;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_REASON_FOR_DISABLE) {
            *value = 0; // BACnetAccessCredentialDisableReason::disabled(0) placeholder; not disabled.
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_CREDENTIAL_DISABLE) {
            *value = 0; // BACnetAccessCredentialDisable::none(0)
            return true;
        }
    }

    // -- Cyan (Access Rights 1) ----------------------------------------------
    // (Enable is Boolean - served by GetPropertyBool below, not here.)

    return false;
}

bool GetPropertyUnsignedInteger(const uint32_t deviceInstance, const uint16_t objectType,
                                const uint32_t objectInstance, const uint32_t propertyIdentifier,
                                uint32_t* value, const bool useArrayIndex,
                                const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)errorCode;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_MULTI_STATE_INPUT && objectInstance == MULTI_STATE_INPUT_INSTANCE) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
            *value = 1;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_NUMBER_OF_STATES) {
            *value = MULTI_STATE_INPUT_NUMBER_OF_STATES;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_STATE_TEXT && useArrayIndex && propertyArrayIndex == 0) {
            *value = MULTI_STATE_INPUT_NUMBER_OF_STATES;
            return true;
        }
    }
    if (objectType == OBJECT_TYPE_DEVICE && objectInstance == g_deviceInstance &&
        propertyIdentifier == PROPERTY_IDENTIFIER_VENDOR_IDENTIFIER) {
        *value = VENDOR_IDENTIFIER;
        return true;
    }
    if (objectType == OBJECT_TYPE_NETWORK_PORT && objectInstance == NETWORK_PORT_INSTANCE) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_APDU_LENGTH) {
            *value = MAX_APDU_LENGTH;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_REFERENCE_PORT) {
            *value = NETWORK_PORT_REFERENCE_PORT_NONE;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_BACNET_IP_UDP_PORT) {
            *value = g_bacnetIpUdpPort;
            return true;
        }
    }
    // Cobalt's required Door timing properties.
    if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_DOOR_PULSE_TIME) { *value = 5; return true; }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_DOOR_EXTENDED_PULSE_TIME) { *value = 15; return true; }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_DOOR_OPEN_TOO_LONG_TIME) { *value = 30; return true; }
    }
    // Copper's required Unsigned properties.
    if (objectType == OBJECT_TYPE_ACCESS_POINT && objectInstance == 1) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_ACCESS_EVENT_TAG) { *value = 0; return true; }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_ACTIVE_AUTHENTICATION_POLICY) { *value = 0; return true; }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_NUMBER_OF_AUTHENTICATION_POLICIES) { *value = 0; return true; }
    }
    // Coral's writable Global_Identifier and Cyan's writable Global_Identifier.
    if ((objectType == OBJECT_TYPE_ACCESS_CREDENTIAL || objectType == OBJECT_TYPE_ACCESS_RIGHTS ||
         objectType == OBJECT_TYPE_ACCESS_ZONE) &&
        objectInstance == 1 && propertyIdentifier == PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER) {
        *value = (objectType == OBJECT_TYPE_ACCESS_CREDENTIAL) ? g_credentialGlobalIdentifier :
                 (objectType == OBJECT_TYPE_ACCESS_RIGHTS) ? g_rightsGlobalIdentifier : g_zoneGlobalIdentifier;
        return true;
    }
    // Ivory (File 1) - File_Size is REQUIRED with no stack default (property-
    // profile-reference.md: "no default...value-not-initialized until a
    // callback or WriteProperty supplies one").
    if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_FILE_SIZE) {
        *value = g_fileDataLength;
        return true;
    }
    return false;
}

// Boolean - Out_Of_Service on every object; Cyan's Enable.
static bool g_accessRightsEnabled = true;

bool GetPropertyBool(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     bool* value, const bool useArrayIndex,
                     const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)errorCode;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRIORITY_ARRAY &&
        useArrayIndex && propertyArrayIndex >= 1 && propertyArrayIndex <= 16) {
        *value = !g_doorIsSet[propertyArrayIndex - 1];
        return true;
    }
    if (propertyIdentifier == PROPERTY_IDENTIFIER_OUT_OF_SERVICE &&
        (objectType == OBJECT_TYPE_ANALOG_INPUT || objectType == OBJECT_TYPE_BINARY_INPUT ||
         objectType == OBJECT_TYPE_MULTI_STATE_INPUT || objectType == OBJECT_TYPE_NETWORK_PORT ||
         objectType == OBJECT_TYPE_ACCESS_DOOR || objectType == OBJECT_TYPE_ACCESS_POINT ||
         objectType == OBJECT_TYPE_ACCESS_ZONE || objectType == OBJECT_TYPE_SCHEDULE)) {
        *value = false;
        return true;
    }
    if (objectType == OBJECT_TYPE_ACCESS_RIGHTS && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_ENABLE) {
        *value = g_accessRightsEnabled;
        return true;
    }
    // Cream (Calendar 1) - Present_Value (required, no stack default): true
    // when today matches an entry in Date_List. Date_List cannot be
    // populated through the customer API (see TODO.md #6 / issue #963), so
    // this honestly always reports false rather than fabricating a match.
    if (objectType == OBJECT_TYPE_CALENDAR && objectInstance == CALENDAR_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
        *value = false;
        return true;
    }
    // Ivory (File 1) - Archive (writable, DM-BR-B configuration-file marker)
    // and Read_Only are both REQUIRED with no stack default.
    if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE) {
        if (propertyIdentifier == PROPERTY_IDENTIFIER_ARCHIVE) {
            *value = g_fileArchive;
            return true;
        }
        if (propertyIdentifier == PROPERTY_IDENTIFIER_READ_ONLY) {
            *value = false; // this example's File is writable (AddFileObject isWritable=true)
            return true;
        }
    }
    return false;
}

bool GetPropertyOctetString(const uint32_t deviceInstance, const uint16_t objectType,
                            const uint32_t objectInstance, const uint32_t propertyIdentifier,
                            uint8_t* value, uint32_t* valueElementCount,
                            const uint32_t maxElementCount, const bool useArrayIndex,
                            const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)errorCode;
    (void)propertyArrayIndex;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_CREDENTIAL_DATA_INPUT && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
        static const uint8_t demoBadgeId[4] = { 0x01, 0x02, 0x03, 0x04 };
        if (maxElementCount < sizeof(demoBadgeId)) {
            return false;
        }
        memcpy(value, demoBadgeId, sizeof(demoBadgeId));
        *valueElementCount = sizeof(demoBadgeId);
        return true;
    }
    if (objectType != OBJECT_TYPE_NETWORK_PORT || objectInstance != NETWORK_PORT_INSTANCE ||
        maxElementCount < 4) {
        return false;
    }
    const uint8_t* source = NULL;
    switch (propertyIdentifier) {
        case PROPERTY_IDENTIFIER_IP_ADDRESS:         source = g_ipAddress; break;
        case PROPERTY_IDENTIFIER_IP_SUBNET_MASK:     source = g_ipSubnetMask; break;
        case PROPERTY_IDENTIFIER_IP_DEFAULT_GATEWAY: source = g_ipDefaultGateway; break;
        default: return false;
    }
    memcpy(value, source, 4);
    *valueElementCount = 4;
    return true;
}

static bool ReturnCharacterString(const char* text, char* value, uint32_t* valueElementCount,
                                  const uint32_t maxElementCount, uint8_t* encodingType) {
    uint32_t length = (uint32_t)strlen(text);
    if (length > maxElementCount) {
        length = maxElementCount;
    }
    memcpy(value, text, length);
    *valueElementCount = length;
    *encodingType = CHARACTER_STRING_ENCODING_UTF8;
    return true;
}

bool GetPropertyCharString(const uint32_t deviceInstance, const uint16_t objectType,
                           const uint32_t objectInstance, const uint32_t propertyIdentifier,
                           char* value, uint32_t* valueElementCount,
                           const uint32_t maxElementCount, uint8_t* encodingType,
                           const bool useArrayIndex, const uint32_t propertyArrayIndex,
                           uint32_t* errorCode) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_MULTI_STATE_INPUT && objectInstance == MULTI_STATE_INPUT_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_STATE_TEXT && useArrayIndex) {
        static const char* const stateText[] = { "On", "Off", "Auto" };
        if (propertyArrayIndex >= 1 && propertyArrayIndex <= MULTI_STATE_INPUT_NUMBER_OF_STATES) {
            return ReturnCharacterString(stateText[propertyArrayIndex - 1], value, valueElementCount,
                                         maxElementCount, encodingType);
        }
        *errorCode = ERROR_CODE_INVALID_ARRAY_INDEX;
        return false;
    }
    if (propertyIdentifier == PROPERTY_IDENTIFIER_OBJECT_NAME) {
        if (objectType == OBJECT_TYPE_DEVICE && objectInstance == g_deviceInstance) {
            return ReturnCharacterString(DEVICE_NAME, value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ANALOG_INPUT && objectInstance == ANALOG_INPUT_INSTANCE) {
            return ReturnCharacterString("Bronze", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_BINARY_INPUT && objectInstance == BINARY_INPUT_INSTANCE) {
            return ReturnCharacterString("Emerald", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_MULTI_STATE_INPUT && objectInstance == MULTI_STATE_INPUT_INSTANCE) {
            return ReturnCharacterString("Hot Pink", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_NETWORK_PORT && objectInstance == NETWORK_PORT_INSTANCE) {
            return ReturnCharacterString("Vermilion", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1) {
            return ReturnCharacterString("Cobalt", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_CREDENTIAL_DATA_INPUT && objectInstance == 1) {
            return ReturnCharacterString("Flax", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ACCESS_POINT && objectInstance == 1) {
            return ReturnCharacterString("Copper", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ACCESS_ZONE && objectInstance == 1) {
            return ReturnCharacterString("Ebony", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ACCESS_CREDENTIAL && objectInstance == 1) {
            return ReturnCharacterString("Coral", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_ACCESS_RIGHTS && objectInstance == 1) {
            return ReturnCharacterString("Cyan", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_EVENT_LOG && objectInstance == EVENT_LOG_INSTANCE) {
            return ReturnCharacterString("Beige", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_SCHEDULE && objectInstance == SCHEDULE_INSTANCE) {
            return ReturnCharacterString("Saffron", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_CALENDAR && objectInstance == CALENDAR_INSTANCE) {
            return ReturnCharacterString("Cream", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE) {
            return ReturnCharacterString("Ivory", value, valueElementCount, maxElementCount, encodingType);
        }
        if (objectType == OBJECT_TYPE_NOTIFICATION_CLASS && objectInstance == NOTIFICATION_CLASS_INSTANCE) {
            return ReturnCharacterString("Crimson", value, valueElementCount, maxElementCount, encodingType);
        }
    }
    // Ivory (File 1) - File_Type is REQUIRED with no stack default.
    if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_FILE_TYPE) {
        return ReturnCharacterString("application/octet-stream", value, valueElementCount,
                                     maxElementCount, encodingType);
    }
    if (objectType == OBJECT_TYPE_DEVICE && objectInstance == g_deviceInstance) {
        switch (propertyIdentifier) {
            case PROPERTY_IDENTIFIER_DESCRIPTION:
                return ReturnCharacterString(DEVICE_DESCRIPTION, value, valueElementCount, maxElementCount, encodingType);
            case PROPERTY_IDENTIFIER_VENDOR_NAME:
                return ReturnCharacterString(VENDOR_NAME, value, valueElementCount, maxElementCount, encodingType);
            case PROPERTY_IDENTIFIER_MODEL_NAME:
                return ReturnCharacterString(MODEL_NAME, value, valueElementCount, maxElementCount, encodingType);
            case PROPERTY_IDENTIFIER_FIRMWARE_REVISION:
                return ReturnCharacterString(g_firmwareRevision.c_str(), value, valueElementCount, maxElementCount, encodingType);
            case PROPERTY_IDENTIFIER_APPLICATION_SOFTWARE_VERSION:
                return ReturnCharacterString(APP_VERSION, value, valueElementCount, maxElementCount, encodingType);
            default:
                break;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// 2a. Flax (Credential Data Input 1) - copied verbatim in shape from B-ACCR's
// canonical CDI pattern (AuthenticationFactor + Supported_Formats + Update_Time).
// See B-ACCR's file header for the full BACnetAuthenticationFactor/
// AuthenticationFactorFormat explanation - not repeated here.
// -----------------------------------------------------------------------------
static uint32_t g_cdiUpdateTimeSeconds = 0; // seconds since start-up; seeded to "now" at start-up (see main())

// Flax's Present_Value (AuthenticationFactor) itself is served by
// GetPropertyOctetString above (see the comment there for why), not here -
// only Supported_Formats needs its own typed callback.
bool GetPropertyAuthenticationFactorFormat(const uint32_t deviceInstance, const uint16_t objectType,
                                           const uint32_t objectInstance, const uint32_t propertyIdentifier,
                                           uint32_t* formatType, bool* useVendorId, uint32_t* vendorId,
                                           bool* useVendorFormat, uint32_t* vendorFormat,
                                           const bool useArrayIndex, const uint32_t propertyArrayIndex,
                                           uint32_t* errorCode) {
    (void)errorCode;
    if (deviceInstance != g_deviceInstance || objectType != OBJECT_TYPE_CREDENTIAL_DATA_INPUT ||
        objectInstance != 1 || propertyIdentifier != PROPERTY_IDENTIFIER_SUPPORTED_FORMATS) {
        return false;
    }
    if (useArrayIndex && propertyArrayIndex == 0) {
        return false; // let the stack answer the ARRAY length (1) itself via its own tracking
    }
    *formatType = 1; // weigand(1) - matches the demo badge bytes in GetPropertyOctetString above
    *useVendorId = false;
    *vendorId = 0;
    *useVendorFormat = false;
    *vendorFormat = 0;
    return true;
}

bool GetPropertyTime(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     uint8_t* hour, uint8_t* minute, uint8_t* second, uint8_t* hundredthSecond,
                     const bool useArrayIndex, const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    (void)errorCode;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_CREDENTIAL_DATA_INPUT && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_UPDATE_TIME) {
        const uint32_t s = g_cdiUpdateTimeSeconds;
        *hour = (uint8_t)((s / 3600) % 24);
        *minute = (uint8_t)((s / 60) % 60);
        *second = (uint8_t)(s % 60);
        *hundredthSecond = 0;
        return true;
    }
    // Ivory (File 1) - Modification_Date's Time half. Fixed at a nominal
    // start-up value; a real device would stamp this on every WriteFile.
    if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_MODIFICATION_DATE) {
        *hour = 0; *minute = 0; *second = 0; *hundredthSecond = 0;
        return true;
    }
    return false;
}

bool GetPropertyDate(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     uint8_t* yearMinus1900, uint8_t* month, uint8_t* day, uint8_t* weekday,
                     const bool useArrayIndex, const uint32_t propertyArrayIndex, uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    (void)errorCode;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_CREDENTIAL_DATA_INPUT && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_UPDATE_TIME) {
        // Update_Time's Date half: this demo tracks only elapsed seconds since
        // start-up (see g_cdiUpdateTimeSeconds), so the date is fixed at
        // start-up - a real device would read its RTC here, as SetSystemTime
        // below would set it.
        *yearMinus1900 = 126; // 2026
        *month = 1;
        *day = 1;
        *weekday = 4; // Thursday
        return true;
    }
    if (objectType == OBJECT_TYPE_FILE && objectInstance == FILE_INSTANCE &&
        propertyIdentifier == PROPERTY_IDENTIFIER_MODIFICATION_DATE) {
        *yearMinus1900 = 126; *month = 1; *day = 1; *weekday = 4;
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// 2b. Property "set" callbacks
// -----------------------------------------------------------------------------

bool SetPropertyEnumerated(const uint32_t deviceInstance, const uint16_t objectType,
                           const uint32_t objectInstance, const uint32_t propertyIdentifier,
                           const uint32_t value, const bool useArrayIndex,
                           const uint32_t propertyArrayIndex, const uint8_t priority,
                           uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    // Cobalt (Access Door 1): WriteProperty(Present_Value, DoorValue, priority) -
    // DS-ACUC-B, the "unlock command" this profile exists for.
    if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
        if (value != DOOR_VALUE_LOCK && value != DOOR_VALUE_UNLOCK) {
            *errorCode = ERROR_CODE_VALUE_OUT_OF_RANGE;
            return false;
        }
        CommandDoorWrite(priority, (double)value);
        printf("WriteProperty: Access Door 1 (Cobalt) <- %s @ priority %u\n",
               value == DOOR_VALUE_UNLOCK ? "unlock(1)" : "lock(0)", EffectivePriority(priority));
        return true;
    }
    // Copper (Access Point 1): WriteProperty(Access_Event, ...) simulates a
    // credential read at the door - see the file header's AE-AC-B discussion
    // for why this drives a ChangeOfState-shaped notification rather than a
    // true ACCESS_EVENT-typed one.
    if (objectType == OBJECT_TYPE_ACCESS_POINT && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_ACCESS_EVENT) {
        g_accessEventValue = value;
        BACnetStack_UpdateValue(g_deviceInstance, OBJECT_TYPE_ACCESS_POINT, 1,
                                PROPERTY_IDENTIFIER_ACCESS_EVENT);
        printf("WriteProperty: Access Point 1 (Copper) Access_Event <- %u %s\n", value,
               value == ACCESS_EVENT_GRANTED ? "(granted)" :
               value == ACCESS_EVENT_DENIED_UNKNOWN_CREDENTIAL ? "(denied - unknown credential)" : "");
        return true;
    }
    // Copper: WriteProperty(Authorization_Mode, ...) - DS-ACSC-B, the
    // supervisory command (e.g. force denyAll for lockdown).
    if (objectType == OBJECT_TYPE_ACCESS_POINT && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_AUTHORIZATION_MODE) {
        if (value != AUTHORIZATION_MODE_AUTHORIZE && value != AUTHORIZATION_MODE_DENY_ALL) {
            *errorCode = ERROR_CODE_VALUE_OUT_OF_RANGE;
            return false;
        }
        g_authorizationMode = value;
        printf("WriteProperty: Access Point 1 (Copper) Authorization_Mode <- %u\n", value);
        return true;
    }
    return false;
}

bool SetPropertyUnsignedInteger(const uint32_t deviceInstance, const uint16_t objectType,
                                const uint32_t objectInstance, const uint32_t propertyIdentifier,
                                const uint32_t value, const bool useArrayIndex,
                                const uint32_t propertyArrayIndex, const uint8_t priority,
                                uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    (void)priority;
    (void)errorCode;
    if (deviceInstance != g_deviceInstance || propertyIdentifier != PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER) {
        return false;
    }
    if (objectType == OBJECT_TYPE_ACCESS_CREDENTIAL && objectInstance == 1) {
        g_credentialGlobalIdentifier = value;
        printf("WriteProperty: Access Credential 1 (Coral) Global_Identifier <- %u\n", value);
        return true;
    }
    if (objectType == OBJECT_TYPE_ACCESS_RIGHTS && objectInstance == 1) {
        g_rightsGlobalIdentifier = value;
        printf("WriteProperty: Access Rights 1 (Cyan) Global_Identifier <- %u\n", value);
        return true;
    }
    if (objectType == OBJECT_TYPE_ACCESS_ZONE && objectInstance == 1) {
        g_zoneGlobalIdentifier = value;
        printf("WriteProperty: Access Zone 1 (Ebony) Global_Identifier <- %u\n", value);
        return true;
    }
    return false;
}

// Ivory (File 1)'s Archive (135-2024 cl. 12.12.4, writable) - the operator
// marks a file as archived; this example just stores the flag.
bool SetPropertyBool(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     const bool value, const bool useArrayIndex,
                     const uint32_t propertyArrayIndex, const uint8_t priority,
                     uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    (void)priority;
    (void)errorCode;
    if (deviceInstance != g_deviceInstance || objectType != OBJECT_TYPE_FILE ||
        objectInstance != FILE_INSTANCE || propertyIdentifier != PROPERTY_IDENTIFIER_ARCHIVE) {
        return false;
    }
    g_fileArchive = value;
    printf("WriteProperty: File 1 (Ivory) Archive <- %s\n", value ? "true" : "false");
    return true;
}

bool SetPropertyNull(const uint32_t deviceInstance, const uint16_t objectType,
                     const uint32_t objectInstance, const uint32_t propertyIdentifier,
                     const bool useArrayIndex, const uint32_t propertyArrayIndex,
                     const uint8_t priority, uint32_t* errorCode) {
    (void)useArrayIndex;
    (void)propertyArrayIndex;
    (void)errorCode;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    if (objectType == OBJECT_TYPE_ACCESS_DOOR && objectInstance == 1 &&
        propertyIdentifier == PROPERTY_IDENTIFIER_PRESENT_VALUE) {
        CommandDoorRelinquish(priority);
        printf("WriteProperty: relinquished Access Door 1 (Cobalt) @ priority %u\n", EffectivePriority(priority));
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Shared password check for DeviceCommunicationControl and ReinitializeDevice.
// -----------------------------------------------------------------------------
static bool PasswordAccepted(const char* password, uint32_t passwordLength) {
    const uint32_t requiredLength = (uint32_t)strlen(DCC_PASSWORD);
    if (requiredLength == 0) {
        return true;
    }
    if (password == NULL || passwordLength != requiredLength) {
        return false;
    }
    unsigned diff = 0;
    for (uint32_t i = 0; i < requiredLength; ++i) {
        diff |= (unsigned)((unsigned char)password[i] ^ (unsigned char)DCC_PASSWORD[i]);
    }
    return diff == 0;
}

bool DeviceCommunicationControl(const uint32_t deviceInstance, const uint8_t enableDisable,
                                const char* password, const uint8_t passwordLength,
                                const bool useTimeDuration, const uint16_t timeDuration,
                                uint32_t* errorCode) {
    if (deviceInstance != g_deviceInstance) {
        *errorCode = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
        return false;
    }
    if (!PasswordAccepted(password, passwordLength)) {
        printf("DeviceCommunicationControl: REJECTED (password failure)\n");
        *errorCode = ERROR_CODE_PASSWORD_FAILURE;
        return false;
    }
    const char* action = (enableDisable == DCC_ENABLE) ? "enable (resume communication)" :
                         (enableDisable == DCC_DISABLE) ? "disable (1) - DEPRECATED, the stack will reject this" :
                         (enableDisable == DCC_DISABLE_INITIATION) ? "disable-initiation (keep responding)" : "unknown";
    if (useTimeDuration) {
        printf("DeviceCommunicationControl: %s for %u minute(s)\n", action, timeDuration);
    } else {
        printf("DeviceCommunicationControl: %s (indefinitely)\n", action);
    }
    return true;
}

// -----------------------------------------------------------------------------
// ReinitializeDevice (DM-RD-B) - ALSO the DM-BR-B backup/restore state machine
// entry point. reinitializedState: 0=COLDSTART, 1=WARMSTART (unchanged from
// B-LSC/B-AAC), 2=STARTBACKUP, 3=ENDBACKUP, 4=STARTRESTORE, 5=ENDRESTORE,
// 6=ABORTRESTORE - these five are new in this file. The stack itself drives
// the backup/restore sequence (calling the four Prepare/Complete callbacks
// below and moving Backup_And_Restore_State) once this callback ACCEPTS the
// request; accepting only means "password OK, proceed" - it does not do the
// backup/restore work itself. B-LSC's copy of this function rejects 2-6 with
// optional-functionality-not-supported because B-LSC does not implement
// DM-BR-B; this file accepts them because this profile does.
// -----------------------------------------------------------------------------
bool ReinitializeDevice(const uint32_t deviceInstance, const uint32_t reinitializedState,
                        const char* password, const uint32_t passwordLength,
                        uint32_t* errorCode) {
    if (deviceInstance != g_deviceInstance) {
        *errorCode = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
        return false;
    }
    if (!PasswordAccepted(password, passwordLength)) {
        printf("ReinitializeDevice: REJECTED (password failure)\n");
        *errorCode = ERROR_CODE_PASSWORD_FAILURE;
        return false;
    }
    if (reinitializedState == REINITIALIZE_STATE_COLDSTART) {
        printf("ReinitializeDevice: COLDSTART accepted (restarting in %u ms)\n",
               (unsigned)CASExampleHelper::RESTART_DELAY_MS);
        CASExampleHelper::RequestRestart(CASExampleHelper::RestartKind::Cold, CASExampleHelper::RESTART_DELAY_MS);
        return true;
    }
    if (reinitializedState == REINITIALIZE_STATE_WARMSTART) {
        printf("ReinitializeDevice: WARMSTART accepted (re-initializing in %u ms)\n",
               (unsigned)CASExampleHelper::RESTART_DELAY_MS);
        CASExampleHelper::RequestRestart(CASExampleHelper::RestartKind::Warm, CASExampleHelper::RESTART_DELAY_MS);
        return true;
    }
    if (reinitializedState == REINITIALIZE_STATE_STARTBACKUP || reinitializedState == REINITIALIZE_STATE_ENDBACKUP ||
        reinitializedState == REINITIALIZE_STATE_STARTRESTORE || reinitializedState == REINITIALIZE_STATE_ENDRESTORE ||
        reinitializedState == REINITIALIZE_STATE_ABORTRESTORE) {
        printf("ReinitializeDevice: backup/restore state %u accepted (DM-BR-B)\n", reinitializedState);
        return true; // the stack's own backup/restore engine takes it from here
    }
    printf("ReinitializeDevice: state %u not supported\n", reinitializedState);
    *errorCode = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
    return false;
}

bool SetSystemTime(const uint32_t deviceInstance, const uint8_t year, const uint8_t month,
                   const uint8_t day, const uint8_t weekday, const uint8_t hour,
                   const uint8_t minute, const uint8_t second, const uint8_t hundrethSeconds) {
    (void)weekday;
    (void)hundrethSeconds;
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    printf("SetSystemTime: %04u-%02u-%02u %02u:%02u:%02u\n", 1900u + year, month, day, hour, minute, second);
    return true;
}

// AcknowledgeAlarm (AE-ACK-B) - accepts an ack for Copper's Access_Event
// notification.
bool AcknowledgeAlarm(const uint32_t deviceInstance, const uint32_t /*acknowledgingProcessIdentifier*/,
                      const uint16_t eventObjectType, const uint32_t eventObjectInstance,
                      const uint16_t /*eventStateAcknowledged*/, const uint8_t /*eventTimeStampYear*/,
                      const uint8_t /*eventTimeStampMonth*/, const uint8_t /*eventTimeStampDay*/,
                      const uint8_t /*eventTimeStampWeekday*/, const uint8_t /*eventTimeStampHour*/,
                      const uint8_t /*eventTimeStampMinute*/, const uint8_t /*eventTimeStampSecond*/,
                      const uint8_t /*eventTimeStampHundrethSecond*/, const char* /*acknowledgementSource*/,
                      const uint32_t /*acknowledgementSourceLength*/, const uint8_t /*acknowledgementSourceEncoding*/,
                      const bool /*timeOfAcknowledgementIsTime*/, const bool /*timeOfAcknowledgementIsSequenceNumber*/,
                      const bool /*timeOfAcknowledgementIsDateTime*/, const uint8_t /*timeOfAcknowledgementYear*/,
                      const uint8_t /*timeOfAcknowledgementMonth*/, const uint8_t /*timeOfAcknowledgementDay*/,
                      const uint8_t /*timeOfAcknowledgementWeekday*/, const uint8_t /*timeOfAcknowledgementHour*/,
                      const uint8_t /*timeOfAcknowledgementMinute*/, const uint8_t /*timeOfAcknowledgementSecond*/,
                      const uint8_t /*timeOfAcknowledgementHundrethSecond*/,
                      const uint16_t /*timeOfAcknowledgementSequenceNumber*/, uint32_t* errorCode) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    const bool isCopper = (eventObjectType == OBJECT_TYPE_ACCESS_POINT && eventObjectInstance == 1);
    if (!isCopper) {
        printf("AcknowledgeAlarm: rejected for object (type %u, instance %u) - no such alarm\n",
               eventObjectType, eventObjectInstance);
        *errorCode = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    printf("AcknowledgeAlarm: Access Point 1 (Copper) Access_Event acknowledged\n");
    return true;
}

// -----------------------------------------------------------------------------
// DM-BR-B: File I/O (backup/restore payload) + the four backup/restore
// lifecycle callbacks. File 1 (Ivory) is a small in-memory STREAM-access file.
// -----------------------------------------------------------------------------
bool ReadFile(const uint32_t deviceInstance, const uint32_t fileInstance, const uint32_t fileStart,
             const uint32_t requestedCount, uint8_t* fileData, uint32_t* fileDataLength,
             const uint32_t maxFileDataLength, bool* endOfFile, uint32_t* errorCode) {
    (void)errorCode;
    if (deviceInstance != g_deviceInstance || fileInstance != FILE_INSTANCE) {
        return false;
    }
    if (fileStart >= g_fileDataLength) {
        *fileDataLength = 0;
        *endOfFile = true;
        return true;
    }
    uint32_t available = g_fileDataLength - fileStart;
    uint32_t toCopy = available;
    if (toCopy > requestedCount) toCopy = requestedCount;
    if (toCopy > maxFileDataLength) toCopy = maxFileDataLength;
    memcpy(fileData, g_fileData + fileStart, toCopy);
    *fileDataLength = toCopy;
    *endOfFile = (fileStart + toCopy >= g_fileDataLength);
    return true;
}

bool WriteFile(const uint32_t deviceInstance, const uint32_t fileInstance, const int32_t fileStart,
              const uint8_t* fileData, const uint32_t fileDataLength, int32_t* ackFileStart,
              uint32_t* errorCode) {
    (void)errorCode;
    if (deviceInstance != g_deviceInstance || fileInstance != FILE_INSTANCE) {
        return false;
    }
    uint32_t start = (fileStart == -1) ? g_fileDataLength : (uint32_t)fileStart;
    if (start + fileDataLength > FILE_MAX_SIZE) {
        return false; // fileFull
    }
    memcpy(g_fileData + start, fileData, fileDataLength);
    if (start + fileDataLength > g_fileDataLength) {
        g_fileDataLength = start + fileDataLength;
    }
    *ackFileStart = (int32_t)start;
    printf("WriteFile: Ivory <- %u byte(s) at offset %d\n", fileDataLength, (int)start);
    return true;
}

bool PrepareBackup(const uint32_t deviceInstance) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    printf("PrepareBackup: staging Ivory's %u byte(s) for AtomicReadFile\n", g_fileDataLength);
    return true; // the demo payload (g_fileData) is already live; nothing more to stage
}
bool CompleteBackup(const uint32_t deviceInstance) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    printf("CompleteBackup: backup session ended\n");
    return true;
}
bool PrepareRestore(const uint32_t deviceInstance) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    printf("PrepareRestore: ready to accept AtomicWriteFile into Ivory\n");
    return true;
}
bool CompleteRestore(const uint32_t deviceInstance, const bool wasAborted) {
    if (deviceInstance != g_deviceInstance) {
        return false;
    }
    printf("CompleteRestore: restore session ended (wasAborted=%s), Ivory now %u byte(s)\n",
           wasAborted ? "true" : "false", g_fileDataLength);
    return true;
}

static void LocalBroadcastConnString(uint8_t out[6]) {
    out[0] = (uint8_t)(g_ipAddress[0] | ~g_ipSubnetMask[0]);
    out[1] = (uint8_t)(g_ipAddress[1] | ~g_ipSubnetMask[1]);
    out[2] = (uint8_t)(g_ipAddress[2] | ~g_ipSubnetMask[2]);
    out[3] = (uint8_t)(g_ipAddress[3] | ~g_ipSubnetMask[3]);
    out[4] = (uint8_t)(g_bacnetIpUdpPort >> 8);
    out[5] = (uint8_t)(g_bacnetIpUdpPort & 0xFF);
}

// -----------------------------------------------------------------------------
// 3. main()
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (!LoadBACnetFunctions()) {
        fprintf(stderr, "Error: failed to load the CAS BACnet Stack: %s\n", CASBACnetStackAdapter_LastError());
        return 1;
    }

    // g_firmwareRevision (Device object property 44) - see its own doc
    // comment above for why this is the STACK's version, not this example's
    // own (that's Application_Software_Version/APP_VERSION instead). Must
    // happen after LoadBACnetFunctions() (these getters ARE some of the
    // functions it loads) and before the Device object is ever readable.
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                 BACnetStack_GetAPIMajorVersion(), BACnetStack_GetAPIMinorVersion(),
                 BACnetStack_GetAPIPatchVersion(), BACnetStack_GetAPIBuildVersion());
        g_firmwareRevision = buf;
    }

    if (CASExampleHelper::HandleHelpAndVersionArgs(argc, argv, APP_NAME, APP_VERSION)) {
        return 0;
    }
    const uint16_t port = CASExampleHelper::ParsePortArg(argc, argv, 47808);
    g_deviceInstance = CASExampleHelper::ParseDeviceIdArg(argc, argv, g_deviceInstance);
    CASExampleHelper::PrintVersion(APP_NAME, APP_VERSION);

    if (!CASExampleHelper::SetupUDP(port)) {
        return 1;
    }
    g_bacnetIpUdpPort = port;
    if (!CASExampleHelper::GetLocalIPv4(g_ipAddress, g_ipSubnetMask)) {
        printf("FYI: could not read a local IPv4 address; Network Port IP_Address will report 0.0.0.0.\n");
    }

    CASExampleHelper::SetNetworkPortInstance(NETWORK_PORT_INSTANCE);
    CASExampleHelper::RegisterCommonCallbacks();
    BACnetStack_RegisterCallbackGetPropertyReal(GetPropertyReal);
    BACnetStack_RegisterCallbackGetPropertyEnumerated(GetPropertyEnumerated);
    BACnetStack_RegisterCallbackGetPropertyUnsignedInteger(GetPropertyUnsignedInteger);
    BACnetStack_RegisterCallbackGetPropertyCharacterString(GetPropertyCharString);
    BACnetStack_RegisterCallbackGetPropertyBool(GetPropertyBool);
    BACnetStack_RegisterCallbackGetPropertyOctetString(GetPropertyOctetString);
    BACnetStack_RegisterCallbackGetPropertyAuthenticationFactorFormat(GetPropertyAuthenticationFactorFormat);
    BACnetStack_RegisterCallbackGetPropertyTime(GetPropertyTime);
    BACnetStack_RegisterCallbackGetPropertyDate(GetPropertyDate);
    BACnetStack_RegisterCallbackSetPropertyEnumerated(SetPropertyEnumerated);
    BACnetStack_RegisterCallbackSetPropertyUnsignedInteger(SetPropertyUnsignedInteger);
    BACnetStack_RegisterCallbackSetPropertyNull(SetPropertyNull);
    BACnetStack_RegisterCallbackSetPropertyBool(SetPropertyBool);
    BACnetStack_RegisterCallbackDeviceCommunicationControl(DeviceCommunicationControl); // DM-DCC-B
    BACnetStack_RegisterCallbackReinitializeDevice(ReinitializeDevice);                 // DM-RD-B + DM-BR-B
    BACnetStack_RegisterCallbackSetSystemTime(SetSystemTime);              // DM-TS-B / DM-UTC-B
    BACnetStack_RegisterCallbackAcknowledgeAlarm(AcknowledgeAlarm);                     // AE-ACK-B
    // DM-BR-B: File I/O + backup/restore lifecycle (all five required together).
    BACnetStack_RegisterCallbackReadFile(ReadFile);
    BACnetStack_RegisterCallbackWriteFile(WriteFile);
    BACnetStack_RegisterCallbackPrepareBackup(PrepareBackup);
    BACnetStack_RegisterCallbackCompleteBackup(CompleteBackup);
    BACnetStack_RegisterCallbackPrepareRestore(PrepareRestore);
    BACnetStack_RegisterCallbackCompleteRestore(CompleteRestore);

    if (!BACnetStack_AddDevice(g_deviceInstance)) {
        printf("Error: Failed to add the Device %u.\n", g_deviceInstance);
        return 1;
    }

    // Services this profile requires. Numbers verified against
    // BACnetServicesSupported.h at the pin (see the runbook's §10 table).
    const struct { uint32_t service; const char* name; } services[] = {
        { SERVICE_READ_PROPERTY,                 "ReadProperty (DS-RP-B)" },
        { SERVICE_READ_PROPERTY_MULTIPLE,        "ReadPropertyMultiple (DS-RPM-B)" },
        { SERVICE_WRITE_PROPERTY,                "WriteProperty (DS-WP-B / DS-ACUC-B / DS-ACSC-B)" },
        { SERVICE_WRITE_PROPERTY_MULTIPLE,       "WritePropertyMultiple (DS-WPM-B)" },
        { SERVICE_SUBSCRIBE_COV,                 "SubscribeCOV (DS-COV-B)" },
        { SERVICE_DEVICE_COMMUNICATION_CONTROL,  "DeviceCommunicationControl (DM-DCC-B)" },
        { SERVICE_REINITIALIZE_DEVICE,           "ReinitializeDevice (DM-RD-B / DM-BR-B)" },
        { SERVICE_TIME_SYNCHRONIZATION,          "TimeSynchronization (DM-TS-B)" },
        { SERVICE_UTC_TIME_SYNCHRONIZATION,      "UTCTimeSynchronization (DM-UTC-B)" },
        { SERVICE_ACKNOWLEDGE_ALARM,             "AcknowledgeAlarm (AE-ACK-B)" },
        { SERVICE_GET_EVENT_INFORMATION,         "GetEventInformation (AE-INFO-B)" },
        { SERVICE_CONFIRMED_EVENT_NOTIFICATION,  "ConfirmedEventNotification (AE-AC-B)" },
        { SERVICE_UNCONFIRMED_EVENT_NOTIFICATION,"UnconfirmedEventNotification (AE-AC-B)" },
        { SERVICE_ATOMIC_READ_FILE,              "AtomicReadFile (DM-BR-B)" },
        { SERVICE_ATOMIC_WRITE_FILE,             "AtomicWriteFile (DM-BR-B)" },
    };
    for (size_t i = 0; i < sizeof(services) / sizeof(services[0]); ++i) {
        if (!BACnetStack_SetServiceEnabled(g_deviceInstance, services[i].service, true)) {
            printf("Error: Failed to enable the %s service.\n", services[i].name);
            return 1;
        }
    }

    if (!BACnetStack_SetServiceEnabled(g_deviceInstance, SERVICE_WHO_IS, true) ||
        !BACnetStack_SetServiceEnabled(g_deviceInstance, SERVICE_I_AM, true) ||
        !BACnetStack_SetServiceEnabled(g_deviceInstance, SERVICE_WHO_HAS, true) ||
        !BACnetStack_SetServiceEnabled(g_deviceInstance, SERVICE_I_HAVE, true)) {
        printf("Error: Failed to enable the discovery services (Who-Is/I-Am, Who-Has/I-Have).\n");
        return 1;
    }

    // --- Base sensor objects --------------------------------------------------
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ANALOG_INPUT, ANALOG_INPUT_INSTANCE)) {
        printf("Error: Failed to add Analog Input %u (Bronze).\n", ANALOG_INPUT_INSTANCE); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_BINARY_INPUT, BINARY_INPUT_INSTANCE)) {
        printf("Error: Failed to add Binary Input %u (Emerald).\n", BINARY_INPUT_INSTANCE); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_MULTI_STATE_INPUT, MULTI_STATE_INPUT_INSTANCE)) {
        printf("Error: Failed to add Multi-State Input %u (Hot Pink).\n", MULTI_STATE_INPUT_INSTANCE); return 1;
    }

    // --- The access family ------------------------------------------------
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1)) {
        printf("Error: Failed to add Access Door 1 (Cobalt).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_CREDENTIAL_DATA_INPUT, 1)) {
        printf("Error: Failed to add Credential Data Input 1 (Flax).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_POINT, 1)) {
        printf("Error: Failed to add Access Point 1 (Copper).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_ZONE, 1)) {
        printf("Error: Failed to add Access Zone 1 (Ebony).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_CREDENTIAL, 1)) {
        printf("Error: Failed to add Access Credential 1 (Coral).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_ACCESS_RIGHTS, 1)) {
        printf("Error: Failed to add Access Rights 1 (Cyan).\n"); return 1;
    }
    if (!BACnetStack_AddEventLogObject(g_deviceInstance, EVENT_LOG_INSTANCE, EVENT_LOG_MAX_BUFFER_SIZE)) {
        printf("Error: Failed to add Event Log 1 (Beige).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_SCHEDULE, SCHEDULE_INSTANCE)) {
        printf("Error: Failed to add Schedule 1 (Saffron).\n"); return 1;
    }
    if (!BACnetStack_AddObject(g_deviceInstance, OBJECT_TYPE_CALENDAR, CALENDAR_INSTANCE)) {
        printf("Error: Failed to add Calendar 1 (Cream).\n"); return 1;
    }
    if (!BACnetStack_AddFileObject(g_deviceInstance, FILE_INSTANCE, true /*isWritable*/,
                                   true /*isConfigurationFile*/, (uint8_t)FILE_ACCESS_METHOD_STREAM)) {
        printf("Error: Failed to add File 1 (Ivory).\n"); return 1;
    }
    if (!BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_FILE, FILE_INSTANCE,
                                         PROPERTY_IDENTIFIER_ARCHIVE, true)) {
        printf("Error: Failed to make Archive writable on File 1 (Ivory).\n"); return 1;
    }

    // --- Network Port -----------------------------------------------------
    if (!BACnetStack_AddNetworkPortObject(g_deviceInstance, NETWORK_PORT_INSTANCE, NETWORK_PORT_NETWORK_TYPE_IPV4,
                                          NETWORK_PORT_PROTOCOL_LEVEL_BACNET_APPLICATION, 0,
                                          NETWORK_NUMBER_QUALITY_UNKNOWN, NETWORK_PORT_REFERENCE_PORT_NONE)) {
        printf("Error: Failed to add Network Port 1 (Vermilion).\n"); return 1;
    }

    // --- Optional properties this example chooses to expose -----------------
    if (!BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_DEVICE, g_deviceInstance,
                                        PROPERTY_IDENTIFIER_DESCRIPTION, true)) {
        printf("Error: Failed to enable Description on the Device object.\n"); return 1;
    }
    if (!BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_MULTI_STATE_INPUT, MULTI_STATE_INPUT_INSTANCE,
                                        PROPERTY_IDENTIFIER_STATE_TEXT, true)) {
        printf("Error: Failed to enable State_Text on Multi-State Input 1 (Hot Pink).\n"); return 1;
    }
    if (!BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                        PROPERTY_IDENTIFIER_DOOR_STATUS, true) ||
        !BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                        PROPERTY_IDENTIFIER_LOCK_STATUS, true) ||
        !BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                        PROPERTY_IDENTIFIER_SECURED_STATUS, true)) {
        printf("Error: Failed to enable the optional Door status properties on Cobalt.\n"); return 1;
    }

    // --- Make Cobalt commandable (DS-ACUC-B) ---------------------------------
    if (!BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                        PROPERTY_IDENTIFIER_PRIORITY_ARRAY, true) ||
        !BACnetStack_SetPropertyEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                        PROPERTY_IDENTIFIER_RELINQUISH_DEFAULT, true) ||
        !BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_DOOR, 1,
                                         PROPERTY_IDENTIFIER_PRESENT_VALUE, true)) {
        printf("Error: Failed to make Access Door 1 (Cobalt) commandable.\n"); return 1;
    }

    // --- Copper: Access_Event writable (demo credential-read trigger) +
    //     Authorization_Mode writable (DS-ACSC-B supervisory command) --------
    if (!BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_POINT, 1,
                                         PROPERTY_IDENTIFIER_ACCESS_EVENT, true) ||
        !BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_POINT, 1,
                                         PROPERTY_IDENTIFIER_AUTHORIZATION_MODE, true)) {
        printf("Error: Failed to make Access_Event/Authorization_Mode writable on Copper.\n"); return 1;
    }

    // --- Coral / Cyan: Global_Identifier writable (both objects) ------------
    if (!BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_CREDENTIAL, 1,
                                         PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER, true) ||
        !BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_RIGHTS, 1,
                                         PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER, true) ||
        !BACnetStack_SetPropertyWritable(g_deviceInstance, OBJECT_TYPE_ACCESS_ZONE, 1,
                                         PROPERTY_IDENTIFIER_GLOBAL_IDENTIFIER, true)) {
        printf("Error: Failed to make Global_Identifier writable on Coral/Cyan/Ebony.\n"); return 1;
    }

    // --- AE-AC-B substitute: Notification Class 1 (Crimson) + the
    //     ChangeOfState algorithm on Copper's Access_Event - see the file
    //     header for why this is the honest substitute for the missing
    //     access-specific intrinsic algorithm. -------------------------------
    if (!BACnetStack_AddNotificationClassObject(g_deviceInstance, NOTIFICATION_CLASS_INSTANCE,
                                                NC_PRIORITY_TO_OFFNORMAL, NC_PRIORITY_TO_FAULT, NC_PRIORITY_TO_NORMAL,
                                                true, false, true)) {
        printf("Error: Failed to add Notification Class 1 (Crimson).\n"); return 1;
    }
    uint8_t recipientMac[6];
    if (RECIPIENT_USE_BROADCAST) {
        LocalBroadcastConnString(recipientMac);
    } else {
        memcpy(recipientMac, RECIPIENT_IP, 4);
        recipientMac[4] = (uint8_t)(g_bacnetIpUdpPort >> 8);
        recipientMac[5] = (uint8_t)(g_bacnetIpUdpPort & 0xFF);
    }
    const uint8_t validDaysAll = 0x7F;
    if (!BACnetStack_AddRecipientToNotificationClass(g_deviceInstance, NOTIFICATION_CLASS_INSTANCE, validDaysAll,
                                                      0, 0, 0, 0, 23, 59, 59, 99, RECIPIENT_PROCESS_IDENTIFIER,
                                                      false, true, true, true, false, 0, true, 0,
                                                      recipientMac, sizeof(recipientMac))) {
        printf("Error: could not seed the Notification Class recipient (Crimson).\n"); return 1;
    }
    if (!BACnetStack_SetAlarmsAndEventsForObjectEnabled(g_deviceInstance, OBJECT_TYPE_ACCESS_POINT, 1,
                                                        NOTIFICATION_CLASS_INSTANCE, NOTIFY_TYPE_ALARM,
                                                        true, true, true, true, true)) {
        printf("Error: could not enable alarms on Copper.\n"); return 1;
    }
    // VERIFIED ABSENT, NOT ASSUMED: BACnetStack_SetIntrinsicChangeOfStateAlgorithmUnsigned
    // itself rejects objectType=accessPoint(33) outright - "intrinsic algorithm
    // changeOfState is not supported by objectType=[33]" (BACnetDBDevice.cpp
    // EnableIntrinsicChangeOfStateAlgorithmUnsigned, confirmed by running this
    // exact call against the pin). So there is no generic-algorithm substitute
    // for AE-AC-B either, on top of SetIntrinsicAccessEventAlgorithm itself being
    // test-tool-only (see the file header). AE-AC-B's alarm GENERATION is left
    // undone - Copper's Access_Event property is still fully read/writable
    // (the file header's credential-read demo still works), but no intrinsic
    // engine watches it and no event notification is produced. See TODO.md #1.

    // --- DS-COV-B: Bronze + Flax's Update_Time -------------------------------
    if (!BACnetStack_SetCOVSettings(g_deviceInstance, COV_MAX_ACTIVE_SUBSCRIPTIONS, COV_MAX_SUPPORTED_LIFETIME_SECONDS) ||
        !BACnetStack_SetMaxActiveCOVSubscriptions(g_deviceInstance, COV_MAX_ACTIVE_SUBSCRIPTIONS)) {
        printf("Error: could not configure COV settings.\n"); return 1;
    }
    if (!BACnetStack_SetPropertySubscribable(g_deviceInstance, OBJECT_TYPE_ANALOG_INPUT, ANALOG_INPUT_INSTANCE,
                                             PROPERTY_IDENTIFIER_PRESENT_VALUE, true) ||
        !BACnetStack_SetPropertySubscribable(g_deviceInstance, OBJECT_TYPE_CREDENTIAL_DATA_INPUT, 1,
                                             PROPERTY_IDENTIFIER_UPDATE_TIME, true)) {
        printf("Error: could not make Bronze/Flax Update_Time COV-subscribable.\n"); return 1;
    }

    // --- DM-BR-B: enable Backup and Restore ---------------------------------
    if (!BACnetStack_SetBackupAndRestoreEnabled(g_deviceInstance, 5 /*backupPreparationTime*/,
                                                5 /*restorePreparationTime*/, 5 /*restoreCompletionTime*/,
                                                120 /*backupFailureTimeout*/, true)) {
        printf("Error: could not enable Backup and Restore (DM-BR-B).\n"); return 1;
    }

    // --- SCHED-I-B: Schedule 1 (Saffron) writes Cobalt's Present_Value -------
    if (!BACnetStack_AddScheduleObject(g_deviceInstance, SCHEDULE_INSTANCE)) {
        printf("Error: Failed to create the stack-held schedule data for Saffron.\n"); return 1;
    }
    if (!BACnetStack_AddScheduleObjectPropertyReference(g_deviceInstance, SCHEDULE_INSTANCE, g_deviceInstance,
                                                         OBJECT_TYPE_ACCESS_DOOR, 1, PROPERTY_IDENTIFIER_PRESENT_VALUE,
                                                         false, 0)) {
        printf("Error: Failed to point Saffron at Access Door 1 (Cobalt).\n"); return 1;
    }
    if (!BACnetStack_SetSchedulePriorityForWriting(g_deviceInstance, SCHEDULE_INSTANCE, SCHEDULE_WRITE_PRIORITY)) {
        printf("Error: Failed to set Saffron's Priority_For_Writing.\n"); return 1;
    }
    if (!BACnetStack_SetScheduleDefault(g_deviceInstance, SCHEDULE_INSTANCE, 9 /*Enumerated*/, DOOR_VALUE_LOCK, 0.0f)) {
        printf("Error: Failed to set Saffron's Schedule_Default.\n"); return 1;
    }
    if (!BACnetStack_SetScheduleEffectivePeriod(g_deviceInstance, SCHEDULE_INSTANCE, 0, 1, 1, 0, 12, 31)) {
        printf("Error: Failed to set Saffron's Effective_Period.\n"); return 1;
    }
    // Every weekday at 08:00, Cobalt unlocks; Schedule_Default (locked) applies
    // the rest of the time. (Weekly_Schedule day order is 0=Monday..6=Sunday.)
    for (uint8_t dayOffset = 0; dayOffset <= 4; ++dayOffset) {
        if (!BACnetStack_AddScheduleWeeklyTimeValue(g_deviceInstance, SCHEDULE_INSTANCE, dayOffset, 8, 0, 0, 0,
                                                    9 /*Enumerated*/, DOOR_VALUE_UNLOCK, 0.0f)) {
            printf("Error: Failed to add Saffron's weekday 08:00 transition.\n"); return 1;
        }
    }
    // One exception: an inline calendar-date entry (Cream's own Date_List
    // cannot be populated through the customer API - same gap B-AAC's file
    // header documents against issue #963; see TODO.md).
    uint32_t exceptionIndex = 0;
    if (!BACnetStack_AddScheduleExceptionEventWithCalendarEntry(g_deviceInstance, SCHEDULE_INSTANCE,
                                                                 0 /*periodType: calendar Date*/, 2026, 12, 25, 255,
                                                                 0, 255, 255, 255, 1 /*eventPriority*/, &exceptionIndex)) {
        printf("Error: Failed to add Saffron's 2026-12-25 exception event.\n"); return 1;
    }
    if (!BACnetStack_AddScheduleExceptionTimeValue(g_deviceInstance, SCHEDULE_INSTANCE, exceptionIndex, 0, 0, 0, 0,
                                                   9 /*Enumerated*/, DOOR_VALUE_LOCK, 0.0f)) {
        printf("Error: Failed to add Saffron's 2026-12-25 exception time-value.\n"); return 1;
    }

    // Seed Flax's Update_Time to "now" so its very first read is meaningful,
    // matching B-ACCR's own start-up convention.
    g_cdiUpdateTimeSeconds = 8 * 3600; // 08:00:00 demo start

    CASExampleHelper::SendIAm(g_deviceInstance);
    uint8_t broadcastConn[6];
    LocalBroadcastConnString(broadcastConn);
    BACnetStack_SendWhoIs(broadcastConn, sizeof(broadcastConn), NETWORK_PORT_INSTANCE, true, 0, NULL, 0);

    printf("FYI: Device %u (\"%s\") ready. Vendor ID %u. Press 'h' for help.\n",
           g_deviceInstance, DEVICE_NAME, VENDOR_IDENTIFIER);

    bool running = true;
    while (running) {
        BACnetStack_Tick();

        CASExampleHelper::RestartKind restartKind;
        if (CASExampleHelper::RestartDue(&restartKind)) {
            if (restartKind == CASExampleHelper::RestartKind::Cold) {
                printf("Restart: COLDSTART - restoring power-on state.\n");
                g_analogInput1Value = 21.5f;
                for (int i = 0; i < 16; ++i) { g_doorIsSet[i] = false; }
                g_accessEventValue = ACCESS_EVENT_NONE;
                g_authorizationMode = AUTHORIZATION_MODE_AUTHORIZE;
            } else {
                printf("Restart: WARMSTART - re-initializing, keeping commanded values.\n");
            }
            CASExampleHelper::SendIAm(g_deviceInstance);
            printf("Restart: complete. Device %u is back.\n", g_deviceInstance);
        }

        switch (CASExampleHelper::PollKey()) {
            case CASExampleHelper::KeyCommand::Help:
                CASExampleHelper::PrintHelp(APP_NAME, APP_VERSION);
                break;
            case CASExampleHelper::KeyCommand::Quit:
                running = false;
                break;
            case CASExampleHelper::KeyCommand::ArrowUp:
                g_analogInput1Value += 1.1f;
                BACnetStack_UpdateValue(g_deviceInstance, OBJECT_TYPE_ANALOG_INPUT, ANALOG_INPUT_INSTANCE,
                                        PROPERTY_IDENTIFIER_PRESENT_VALUE);
                printf("Analog Input 1 (Bronze) = %.1f C\n", g_analogInput1Value);
                break;
            case CASExampleHelper::KeyCommand::ArrowDown:
                g_analogInput1Value -= 1.1f;
                BACnetStack_UpdateValue(g_deviceInstance, OBJECT_TYPE_ANALOG_INPUT, ANALOG_INPUT_INSTANCE,
                                        PROPERTY_IDENTIFIER_PRESENT_VALUE);
                printf("Analog Input 1 (Bronze) = %.1f C\n", g_analogInput1Value);
                break;
            case CASExampleHelper::KeyCommand::DemoAdvance:
                // Claimed by B-AAC's Schedule demo - has no effect here; see the
                // file header's note on why this example uses WriteProperty
                // instead of claiming its own key.
                break;
            case CASExampleHelper::KeyCommand::None:
            default:
                break;
        }

#if defined(_WIN32)
        Sleep(1);
#else
        usleep(1000);
#endif
    }

    CASExampleHelper::RestoreInput();
    CASExampleHelper::ShutdownUDP();
    return 0;
}
