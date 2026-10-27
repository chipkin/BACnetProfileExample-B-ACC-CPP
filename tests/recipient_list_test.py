#!/usr/bin/env python3
# SPDX-License-Identifier: CC0-1.0
# Public-domain example code (CC0) - see ../LICENSE.
"""B-ACC Wire Test: Notification Class 1's Recipient_List is writable.

    python3 tests/recipient_list_test.py [--exe build/BACnetExampleBACC] [--port 47829]

Crimson's Recipient_List (135-2024 cl. 12.21.8) starts with one entry that main.cpp seeds:
the local subnet's broadcast address, Process Identifier 1, unconfirmed, every day, all day,
every transition. The list is writable, so a management station can direct the alarms to its
own recipients; a read-only list would have to be the single unconfirmed local-broadcast entry
with Process Identifier 0 that cl. 12.21 defines.

  1. The default list reads back as one BACnetDestination.
  2. A WriteProperty of a two-entry list (Device 389110, Process Identifier 2, confirmed;
     Device 389111, Process Identifier 3, unconfirmed, to-offnormal only) is accepted and
     reads back unchanged.
  3. Writing the default back is accepted and reads back unchanged.

Not tested here: delivery. This Blocked Example generates no event notification of any kind
(TODO.md #1), so no recipient ever receives one.

Exit code 0 = every check passed.
"""
import argparse
import sys
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPOSITORY))

import wiretest  # noqa: E402  (the vendored harness at the repository root)
from wiretest import codec, record, summary  # noqa: E402

OBJECT_TYPE_DEVICE = 8
OBJECT_TYPE_NOTIFICATION_CLASS = 15
CRIMSON = (OBJECT_TYPE_NOTIFICATION_CLASS, 1)
PROPERTY_RECIPIENT_LIST = 102


def destination(device_instance, process_identifier, confirmed, transitions=(True, True, True)):
    """One BACnetDestination (cl. 21): all days, 00:00:00.00 - 23:59:59.99, the device choice."""
    transition_bits = sum(0x80 >> i for i, on in enumerate(transitions) if on)
    return (bytes([0x82, 0x01, 0xFE])                       # Valid_Days: BIT STRING, 7 bits, all set
            + bytes([0xB4, 0, 0, 0, 0])                    # From_Time 00:00:00.00
            + bytes([0xB4, 23, 59, 59, 99])                # To_Time 23:59:59.99
            + codec.context_object_id(0, OBJECT_TYPE_DEVICE, device_instance)   # Recipient: device [0]
            + codec.application_unsigned(process_identifier)
            + codec.application_boolean(confirmed)
            + bytes([0x82, 0x05, transition_bits]))        # Transitions: BIT STRING, 3 bits


TWO_ENTRY_LIST = destination(389110, 2, True) + destination(389111, 3, False, (True, False, False))


def parser():
    arguments = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    arguments.add_argument("--exe", default=str(wiretest.default_exe(REPOSITORY, "BACnetExampleBACC")),
                           help="path to BACnetExampleBACC (default: build/BACnetExampleBACC)")
    arguments.add_argument("--port", type=int, default=47829,
                           help="the access control controller's UDP port (default 47829)")
    return arguments


def acked(answer):
    return answer.answered and answer.frame.pdu_type == wiretest.PDU_SIMPLE_ACK


def read_list(client, port):
    answer = client.read_property(port, *CRIMSON, PROPERTY_RECIPIENT_LIST, timeout=2.0)
    return answer, (answer.value_octets if answer.answered and answer.error is None else None)


def main():
    args = parser().parse_args()
    controller = wiretest.start(args.exe, ["--port", args.port], "B-ACC")
    client = wiretest.Client()
    try:
        answer, default = read_list(client, args.port)
        # One BACnetDestination starts with the Valid_Days BIT STRING; a second one would start
        # again after the first one's Transitions BIT STRING (82 05 ..).
        record("Recipient_List reads the default: one BACnetDestination",
               default is not None and default.startswith(bytes([0x82, 0x01, 0xFE]))
               and default.count(bytes([0x82, 0x05])) == 1,
               default.hex() if default else answer.describe())

        for label, value in (("a two-entry list", TWO_ENTRY_LIST), ("the default back", default or b"")):
            written = client.write_property(args.port, *CRIMSON, PROPERTY_RECIPIENT_LIST, value)
            record(f"WriteProperty Recipient_List with {label} is accepted", acked(written), written.describe())
            answer, octets = read_list(client, args.port)
            record("  ... and reads back unchanged", octets == value, octets.hex() if octets else answer.describe())
    finally:
        client.close()
        controller.stop()
    return summary()


if __name__ == "__main__":
    sys.exit(main())
