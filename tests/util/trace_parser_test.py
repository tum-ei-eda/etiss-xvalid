# SPDX-License-Identifier: Apache-2.0
import struct
import tempfile
import unittest
from pathlib import Path

from py_etiss_xvalid.util.trace_parser import parse_trace_file


def _memory_access_record(entry_type, pc, addr, data):
    return struct.pack(
        "<IIQI64s",
        entry_type,
        pc,
        addr,
        len(data),
        data + bytes(64 - len(data)),
    )


class TestTraceParser(unittest.TestCase):
    def test_trace_parser_accepts_state_write_and_read_records(self):
        with tempfile.TemporaryDirectory() as test_dir:
            trace_path = Path(test_dir) / "trace.bin"
            state_snapshot = struct.pack(
                "<III32I32Q16s",
                1,
                0x100,
                0x80000000,
                *range(32),
                *range(32),
                b"cswsp",
            )
            dwrite = _memory_access_record(2, 0x104, 0x20000000, bytes.fromhex("01020304"))
            dread = _memory_access_record(3, 0x108, 0x20000004, bytes.fromhex("AABB"))
            trace_path.write_bytes(state_snapshot + dwrite + dread)

            entries = parse_trace_file(trace_path)

        self.assertEqual(
            [
                {
                    "type": "state_snapshot",
                    "pc": 0x100,
                    "sp": 0x80000000,
                    "x": list(range(32)),
                    "f": list(range(32)),
                    "instruction": "cswsp",
                },
                {
                    "type": "dwrite",
                    "pc": 0x104,
                    "location": "20000000",
                    "byte_size": 4,
                    "data": "01020304",
                },
                {
                    "type": "dread",
                    "pc": 0x108,
                    "location": "20000004",
                    "byte_size": 2,
                    "data": "AABB",
                },
            ],
            entries,
        )
