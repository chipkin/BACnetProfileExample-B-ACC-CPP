#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
# Public-domain example code (CC0) - see ../LICENSE.
"""B-ACC discovery: read every property the access control controller says it has, the way a client does.

    python3 tests/discovery_test.py [--exe build/BACnetExampleBACC] [--port 47829]

What a BACnet client such as Chipkin BACnet Explorer does when it discovers a device,
done over the wire from outside the example:

    1. Who-Is   -> the I-Am of Device 389009
    2. ReadProperty Device Object_List
    3. for every object: ReadProperty Property_List, then ReadProperty of every
       property it lists (and of Object_Identifier, Object_Name, Object_Type and
       Property_List itself, which cl. 12.1.1.4.1 leaves out of the list)

Every one of those reads must come back as a value - a ReadProperty ComplexACK. An
Error, Reject or Abort, or no answer, fails the check: Property_List holds "one property
identifier for each property that exists within the object" (135-2024 cl. 12.1.1.4.1),
so a listed property the device cannot read is a defect, whoever owns it.

The three reads named separately are the ones Explorer 4.23.2.0 found failing on
Network Port 1 at stack 6.0.21 (reported against B-SS-CPP as issues #20, #21, #22; the
same Network Port setup is used here).

Exit code 0 = every check passed.
"""
import argparse
import sys
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY))

import wiretest  # noqa: E402  (the vendored harness at the repository root)
from wiretest import known_gap, record, summary  # noqa: E402

# Match main.cpp.
DEVICE_INSTANCE = 389009
DEVICE = (wiretest.OBJECT_TYPE_DEVICE, DEVICE_INSTANCE)
NETWORK_PORT = (wiretest.OBJECT_TYPE_NETWORK_PORT, 1)            # Network Port 1 "Vermilion"
# Object types wiretest does not name (cl. 21 BACnetObjectType).
OBJECT_TYPE_CALENDAR = 6
OBJECT_TYPE_FILE = 10
OBJECT_TYPE_NOTIFICATION_CLASS = 15
OBJECT_TYPE_SCHEDULE = 17
OBJECT_TYPE_EVENT_LOG = 25
OBJECT_TYPE_ACCESS_DOOR = 30
OBJECT_TYPE_ACCESS_CREDENTIAL = 32
OBJECT_TYPE_ACCESS_POINT = 33
OBJECT_TYPE_ACCESS_RIGHTS = 34
OBJECT_TYPE_ACCESS_ZONE = 36
OBJECT_TYPE_CREDENTIAL_DATA_INPUT = 37
EXPECTED_OBJECTS = [DEVICE,
                    (wiretest.OBJECT_TYPE_ANALOG_INPUT, 1),          # "Bronze"
                    (wiretest.OBJECT_TYPE_BINARY_INPUT, 1),          # "Emerald"
                    (wiretest.OBJECT_TYPE_MULTI_STATE_INPUT, 1),     # "Hot Pink"
                    (OBJECT_TYPE_ACCESS_DOOR, 1),                    # "Cobalt"
                    (OBJECT_TYPE_CREDENTIAL_DATA_INPUT, 1),          # "Flax"
                    (OBJECT_TYPE_ACCESS_POINT, 1),                   # "Copper"
                    (OBJECT_TYPE_ACCESS_ZONE, 1),                    # "Ebony"
                    (OBJECT_TYPE_ACCESS_CREDENTIAL, 1),              # "Coral"
                    (OBJECT_TYPE_ACCESS_RIGHTS, 1),                  # "Cyan"
                    (OBJECT_TYPE_EVENT_LOG, 1),                      # "Beige"
                    (OBJECT_TYPE_SCHEDULE, 1),                       # "Saffron"
                    (OBJECT_TYPE_CALENDAR, 1),                       # "Cream"
                    (OBJECT_TYPE_FILE, 1),                           # "Ivory"
                    (OBJECT_TYPE_NOTIFICATION_CLASS, 1),             # "Crimson"
                    NETWORK_PORT]

# The four properties cl. 12.1.1.4.1 says Property_List does not contain.
PROPERTY_OBJECT_TYPE = 79
NOT_IN_PROPERTY_LIST = [wiretest.PROPERTY_OBJECT_IDENTIFIER, wiretest.PROPERTY_OBJECT_NAME,
                        PROPERTY_OBJECT_TYPE, wiretest.PROPERTY_PROPERTY_LIST]

# Network Port properties Explorer could not read at stack 6.0.21 (cl. 12.56; numbers from
# cl. 21 BACnetPropertyIdentifier), with the issue that reported each.
PROPERTY_IP_DNS_SERVER = 406
PROPERTY_FD_BBMD_ADDRESS = 418
PROPERTY_FD_SUBSCRIPTION_LIFETIME = 419
EXPLORER_FAILURES = [(PROPERTY_IP_DNS_SERVER, "IP_DNS_Server", "B-SS-CPP #20"),
                     (PROPERTY_FD_BBMD_ADDRESS, "FD_BBMD_Address", "B-SS-CPP #21"),
                     (PROPERTY_FD_SUBSCRIPTION_LIFETIME, "FD_Subscription_Lifetime", "B-SS-CPP #22")]

OBJECT_TYPE_NAMES = {wiretest.OBJECT_TYPE_ANALOG_INPUT: "Analog Input", wiretest.OBJECT_TYPE_BINARY_INPUT: "Binary Input",
                     wiretest.OBJECT_TYPE_DEVICE: "Device", wiretest.OBJECT_TYPE_MULTI_STATE_INPUT: "Multi-state Input",
                     wiretest.OBJECT_TYPE_NETWORK_PORT: "Network Port",
                     OBJECT_TYPE_CALENDAR: "Calendar",
                     OBJECT_TYPE_FILE: "File",
                     OBJECT_TYPE_NOTIFICATION_CLASS: "Notification Class",
                     OBJECT_TYPE_SCHEDULE: "Schedule",
                     OBJECT_TYPE_EVENT_LOG: "Event Log",
                     OBJECT_TYPE_ACCESS_DOOR: "Access Door",
                     OBJECT_TYPE_ACCESS_CREDENTIAL: "Access Credential",
                     OBJECT_TYPE_ACCESS_POINT: "Access Point",
                     OBJECT_TYPE_ACCESS_RIGHTS: "Access Rights",
                     OBJECT_TYPE_ACCESS_ZONE: "Access Zone",
                     OBJECT_TYPE_CREDENTIAL_DATA_INPUT: "Credential Data Input"}
PDU_NAMES = {wiretest.PDU_ERROR: "Error", wiretest.PDU_REJECT: "Reject", wiretest.PDU_ABORT: "Abort"}

# The one listed property ReadProperty must refuse. Event Log's Log_Buffer: "The log buffer is not
# network accessible except through the use of the ReadRange service" (135-2024 cl. 12.27.13), and
# ReadProperty answers "The property is not accessible using this service" with PROPERTY /
# READ_ACCESS_DENIED (cl. 15.5.1.3.1). So that Error is the expected answer and anything else,
# a value included, fails. (Added to the B-SS-CPP da3e96f test, which has no log object.)
PROPERTY_LOG_BUFFER = 131
ERROR_CLASS_PROPERTY = 2
ERROR_CODE_READ_ACCESS_DENIED = 27
READ_RANGE_ONLY = {(OBJECT_TYPE_EVENT_LOG, PROPERTY_LOG_BUFFER): "Log_Buffer"}

# Listed properties the stack cannot serve through the customer interface yet - a defect outside
# this example, filed. Each is reported as a [KNOWN GAP] on every run and does not fail it; when one
# reads as a value it prints [GAP CLOSED]: make it a checked requirement and drop the TODO.md entry.
KNOWN_GAPS = {
    ((OBJECT_TYPE_ACCESS_CREDENTIAL, 1), 256): ("Assigned_Access_Rights", "stack #2150"),
    ((OBJECT_TYPE_ACCESS_CREDENTIAL, 1), 257): ("Authentication_Factors", "stack #2150"),
    ((OBJECT_TYPE_ACCESS_POINT, 1), 246): ("Access_Doors", "stack #2150"),
    ((OBJECT_TYPE_ACCESS_POINT, 1), 249): ("Access_Event_Credential", "stack #2150"),
    ((OBJECT_TYPE_ACCESS_RIGHTS, 1), 288): ("Negative_Access_Rules", "stack #2150"),
    ((OBJECT_TYPE_ACCESS_RIGHTS, 1), 302): ("Positive_Access_Rules", "stack #2150"),
}


def parser():
    arguments = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    arguments.add_argument("--exe", default=str(wiretest.default_exe(REPOSITORY, "BACnetExampleBACC")),
                           help="path to BACnetExampleBACC (default: build/BACnetExampleBACC)")
    arguments.add_argument("--port", type=int, default=47829,
                           help="the access control controller's UDP port (default 47829)")
    return arguments


def object_text(object_identifier):
    object_type, instance = object_identifier
    return f"{OBJECT_TYPE_NAMES.get(object_type, f'object type {object_type}')} {instance}"


def outcome(answer):
    """What a ReadProperty got back, in words: the value, or why there is none."""
    if not answer.answered:
        return "no answer"
    if answer.frame.pdu_type == wiretest.PDU_COMPLEX_ACK:
        return f"value {answer.value!r}"
    if answer.error is not None:
        return f"Error class={answer.error[0]} code={answer.error[1]}"
    if answer.frame.pdu_type in (wiretest.PDU_REJECT, wiretest.PDU_ABORT) and len(answer.frame.payload) > 2:
        return f"{PDU_NAMES[answer.frame.pdu_type]} reason={answer.frame.payload[2]}"
    return answer.describe()


def is_value(answer):
    return answer.answered and answer.frame.pdu_type == wiretest.PDU_COMPLEX_ACK


def who_is(client, port, attempts=2):
    """Who-Is for this one device, unicast to it (a client that already knows the address
    does this; Explorer broadcasts). Returns the I-Am frame that comes back, or None.
    Who-Is is unconfirmed, so nothing else retries it: if no I-Am comes back it is sent
    once more, with a visible [RETRY] line (the example's UDP receive can drop a datagram
    on Windows - common/ SimpleUDP)."""
    for attempt in range(attempts):
        if attempt:
            print("[RETRY] Who-Is got no I-Am in 3.0 s - sent again", flush=True)
        client.frames.clear()
        client.send(wiretest.bvlc(wiretest.npdu(wiretest.who_is(DEVICE_INSTANCE, DEVICE_INSTANCE))), port)
        frame = client.wait_for(lambda candidate: wiretest.i_am_device_instance(candidate.payload) == DEVICE_INSTANCE,
                                3.0)
        if frame is not None:
            return frame
    return None


def main():
    args = parser().parse_args()
    sensor = wiretest.start(args.exe, ["--port", args.port], "B-ACC")
    client = wiretest.Client(retries=1)
    try:
        i_am = who_is(client, args.port)
        record(f"Who-Is: Device {DEVICE_INSTANCE} answers with an I-Am", i_am is not None,
               "no I-Am" if i_am is None else "")

        object_list_answer = client.read_property(args.port, *DEVICE, wiretest.PROPERTY_OBJECT_LIST, timeout=2.0)
        object_list = wiretest.decoded_values(object_list_answer.value_octets) if is_value(object_list_answer) else []
        # Control: the walk below proves nothing if it walks nothing.
        record("Device Object_List names the Device, the three sensors, the six access objects, Event Log, Schedule, "
               "Calendar, File, Notification Class and Network Port 1 (16 objects)", sorted(object_list) == sorted(EXPECTED_OBJECTS),
               f"{len(object_list)} objects" if is_value(object_list_answer) else outcome(object_list_answer))

        reads = 0
        unreadable = {}
        property_lists = {}
        for object_identifier in object_list:
            list_answer = client.read_property(args.port, *object_identifier, wiretest.PROPERTY_PROPERTY_LIST,
                                               timeout=2.0)
            property_list = wiretest.decoded_values(list_answer.value_octets) if is_value(list_answer) else []
            property_lists[object_identifier] = property_list
            record(f"{object_text(object_identifier)}: Property_List reads", is_value(list_answer) and property_list,
                   outcome(list_answer) if not is_value(list_answer) else f"{len(property_list)} properties")
            for property_identifier in NOT_IN_PROPERTY_LIST + property_list:
                answer = client.read_property(args.port, *object_identifier, property_identifier, timeout=2.0)
                reads += 1
                read_range_only = READ_RANGE_ONLY.get((object_identifier[0], property_identifier))
                if read_range_only:
                    record(f"{object_text(object_identifier)} {read_range_only} ({property_identifier}): ReadProperty "
                           f"answers Error property/read-access-denied (cl. 12.27.13: ReadRange only)",
                           answer.answered and answer.error == (ERROR_CLASS_PROPERTY, ERROR_CODE_READ_ACCESS_DENIED),
                           outcome(answer))
                    continue
                gap = KNOWN_GAPS.get((object_identifier, property_identifier))
                if gap:
                    known_gap(f"{object_text(object_identifier)} {gap[0]} ({property_identifier}) reads as a value",
                              is_value(answer), f"{outcome(answer)}; {gap[1]}")
                    continue
                if not is_value(answer):
                    unreadable[(object_identifier, property_identifier)] = outcome(answer)
                    print(f"[UNREADABLE] {object_text(object_identifier)} property {property_identifier}: "
                          f"{outcome(answer)}", flush=True)

        record(f"every property of every object reads as a value ({reads} reads)", reads and not unreadable,
               f"{len(unreadable)} unreadable: " + ", ".join(
                   f"{object_text(object_identifier)} {property_identifier}"
                   for object_identifier, property_identifier in unreadable) if unreadable else "")

        # The three Explorer reported, by name - listed in Property_List and readable.
        network_port_list = property_lists.get(NETWORK_PORT, [])
        for property_identifier, name, issue in EXPLORER_FAILURES:
            listed = property_identifier in network_port_list
            failure = unreadable.get((NETWORK_PORT, property_identifier))
            record(f"Network Port 1 {name} ({property_identifier}): listed in Property_List and reads as a value "
                   f"(issue {issue})", listed and failure is None,
                   "not in Property_List" if not listed else (failure or ""))
    finally:
        client.close()
        sensor.stop()
    return summary()


if __name__ == "__main__":
    sys.exit(main())
