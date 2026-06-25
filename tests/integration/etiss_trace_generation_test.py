# SPDX-License-Identifier: Apache-2.0
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

from py_etiss_xvalid.util.trace_parser import parse_trace_file


SMOKE_ASM = """
    .section .text
    .globl _start
_start:
    lui sp, 0x8
    c.swsp ra, 0(sp)
    lui t0, %hi(sample)
    lw t1, %lo(sample)(t0)
    sw t1, %lo(sample_out)(t0)
    li t2, 0x20
    c.jr t2
    .org 0x20
done:
    j done

    .section .data
    .balign 4
sample:
    .word 0x11223344
sample_out:
    .word 0
"""

SMOKE_INI = """
[StringConfigurations]
etiss_wd={work_dir}
vp.elf_file={elf_path}
arch.cpu=RV32IMACFD
jit.type=TCCJIT

[BoolConfigurations]
etiss.exit_on_loop=true
arch.enable_semihosting=true
etiss.load_integrated_libraries=true
jit.gcc.cleanup=true
jit.verify=false
vp.quiet=true

[IntConfigurations]
etiss.loglevel=2
etiss.max_block_size=100
arch.cpu_cycle_time_ps=31250
simple_mem_system.memseg_origin_00=0x00000000
simple_mem_system.memseg_length_00=0x00080000
simple_mem_system.memseg_origin_01=0x00080000
simple_mem_system.memseg_length_01=0x00080000
"""


@unittest.skipUnless(
    os.environ.get("ETISS_XVALID_RUN_INTEGRATION") == "1",
    "ETISS integration test is opt-in",
)
class TestEtissTraceGeneration(unittest.TestCase):
    def setUp(self):
        clang = shutil.which("clang")
        self.assertIsNotNone(clang, "clang is required for the integration smoke ELF")

        self.clang = clang
        self.etiss_install = Path(os.environ["ETISS_INSTALL_DIR"])
        self.xvalid_install = Path(os.environ["XVALID_INSTALL_DIR"])
        self.bare_etiss = self.etiss_install / "bin" / "bare_etiss_processor"
        self.xvalid_plugin = self.xvalid_install / "lib" / "plugins" / "libXValid.so"
        self.assertTrue(self.bare_etiss.exists(), self.bare_etiss)
        self.assertTrue(self.xvalid_plugin.exists(), self.xvalid_plugin)

    def test_bare_etiss_processor_generates_read_and_write_trace_records(self):
        with tempfile.TemporaryDirectory() as test_dir:
            work_dir, ini_path, trace_path = self._prepare_smoke_program(Path(test_dir))
            (work_dir / "pcs.tmp").write_text("0;128\n")

            result = self._run_bare_etiss(
                work_dir,
                [
                    f"-i{ini_path}",
                    "-p",
                    "GTS",
                    "DataWriteTracer",
                    "DataReadTracer",
                ],
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertTrue(trace_path.exists(), result.stdout)

            entries = parse_trace_file(trace_path)

        self.assertTrue(any(entry["type"] == "state_snapshot" for entry in entries), entries)
        self.assertTrue(
            any(
                entry["type"] == "dread"
                and entry["location"] == "00001000"
                and entry["byte_size"] == 4
                and entry["data"] == "44332211"
                for entry in entries
            ),
            entries,
        )

    def test_instruction_tracer_accepts_direct_pc_range_option(self):
        with tempfile.TemporaryDirectory() as test_dir:
            work_dir, ini_path, trace_path = self._prepare_smoke_program(Path(test_dir))

            result = self._run_bare_etiss(
                work_dir,
                [
                    f"-i{ini_path}",
                    "-p",
                    "GTS",
                    "DataWriteTracer",
                    "DataReadTracer",
                    "--plugin.instruction_tracer.pc_range=0x0:0x80,0x200:0x210",
                ],
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertFalse((work_dir / "pcs.tmp").exists(), result.stdout)
            self.assertTrue(trace_path.exists(), result.stdout)

            entries = parse_trace_file(trace_path)

        self.assertTrue(any(entry["type"] == "state_snapshot" for entry in entries), entries)
        self.assertTrue(any(entry["type"] == "dread" for entry in entries), entries)
        self.assertTrue(any(entry["type"] == "dwrite" for entry in entries), entries)

    def test_isa_extension_validator_accepts_instruction_and_pc_filters(self):
        with tempfile.TemporaryDirectory() as test_dir:
            work_dir, ini_path, _ = self._prepare_smoke_program(Path(test_dir))

            result = self._run_bare_etiss(
                work_dir,
                [
                    f"-i{ini_path}",
                    "-p",
                    "ISAExtensionValidator",
                    "--plugin.isa_extension_validator.instructions=cjr,cswsp",
                    "--plugin.isa_extension_validator.pc_range=0x0:0x10",
                ],
            )

        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn("X[PC]: 4", result.stdout)
        self.assertNotIn("X[PC]: 22", result.stdout)

    def _prepare_smoke_program(self, work_dir):
        plugin_dir = work_dir / "PluginImpl"
        plugin_dir.mkdir()
        (plugin_dir / "libXValid.so").symlink_to(self.xvalid_plugin)

        asm_path = work_dir / "trace_smoke.S"
        elf_path = work_dir / "trace_smoke.elf"
        ini_path = work_dir / "trace_smoke.ini"
        trace_path = work_dir / "trace.bin"
        asm_path.write_text(SMOKE_ASM)

        subprocess.run(
            [
                self.clang,
                "--target=riscv32",
                "-march=rv32imac",
                "-mabi=ilp32",
                "-nostdlib",
                "-fuse-ld=lld",
                "-Wl,-Ttext=0x0",
                "-Wl,-Tdata=0x1000",
                "-Wl,-e,_start",
                str(asm_path),
                "-o",
                str(elf_path),
            ],
            check=True,
            cwd=work_dir,
        )
        ini_path.write_text(SMOKE_INI.format(work_dir=work_dir, elf_path=elf_path))
        return work_dir, ini_path, trace_path

    def _run_bare_etiss(self, work_dir, args):
        env = os.environ.copy()
        env["LD_LIBRARY_PATH"] = ":".join(
            [
                str(self.etiss_install / "lib"),
                str(self.xvalid_install / "lib"),
                str(self.xvalid_install / "lib" / "plugins"),
                env.get("LD_LIBRARY_PATH", ""),
            ]
        )
        return subprocess.run(
            [str(self.bare_etiss), *args],
            cwd=work_dir,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=5,
        )
        self.assertTrue(
            any(
                entry["type"] == "dwrite"
                and entry["location"] == "00001004"
                and entry["byte_size"] == 4
                and entry["data"] == "44332211"
                for entry in entries
            ),
            entries,
        )
