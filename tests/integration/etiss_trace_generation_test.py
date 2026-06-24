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


@unittest.skipUnless(
    os.environ.get("ETISS_XVALID_RUN_INTEGRATION") == "1",
    "ETISS integration test is opt-in",
)
class TestEtissTraceGeneration(unittest.TestCase):
    def test_bare_etiss_processor_generates_read_and_write_trace_records(self):
        clang = shutil.which("clang")
        self.assertIsNotNone(clang, "clang is required for the integration smoke ELF")

        etiss_install = Path(os.environ["ETISS_INSTALL_DIR"])
        xvalid_install = Path(os.environ["XVALID_INSTALL_DIR"])
        bare_etiss = etiss_install / "bin" / "bare_etiss_processor"
        xvalid_plugin = xvalid_install / "lib" / "plugins" / "libXValid.so"
        self.assertTrue(bare_etiss.exists(), bare_etiss)
        self.assertTrue(xvalid_plugin.exists(), xvalid_plugin)

        with tempfile.TemporaryDirectory() as test_dir:
            work_dir = Path(test_dir)
            plugin_dir = work_dir / "PluginImpl"
            plugin_dir.mkdir()
            (plugin_dir / "libXValid.so").symlink_to(xvalid_plugin)

            asm_path = work_dir / "trace_smoke.S"
            elf_path = work_dir / "trace_smoke.elf"
            trace_path = work_dir / "trace.bin"
            asm_path.write_text(SMOKE_ASM)
            (work_dir / "pcs.tmp").write_text("0;128\n")

            subprocess.run(
                [
                    clang,
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

            env = os.environ.copy()
            env["LD_LIBRARY_PATH"] = ":".join(
                [
                    str(etiss_install / "lib"),
                    str(xvalid_install / "lib"),
                    str(xvalid_install / "lib" / "plugins"),
                    env.get("LD_LIBRARY_PATH", ""),
                ]
            )

            result = subprocess.run(
                [
                    str(bare_etiss),
                    f"--vp.elf_file={elf_path}",
                    "--arch.cpu=RV32IMACFD",
                    "--jit.type=TCCJIT",
                    "--etiss.exit_on_loop=true",
                    "--vp.quiet=true",
                    f"--etiss_wd={work_dir}",
                    "-p",
                    "GTS",
                    "DataWriteTracer",
                    "DataReadTracer",
                    "--plugin.data_write_tracer.logaddr=0x1000",
                    "--plugin.data_write_tracer.logmask=0xfffffff0",
                    "--plugin.data_read_tracer.logaddr=0x1000",
                    "--plugin.data_read_tracer.logmask=0xfffffff0",
                ],
                cwd=work_dir,
                env=env,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
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
