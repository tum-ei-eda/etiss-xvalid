# ETISS XValid

Out-of-tree ETISS plugins and Python tooling for the XValid validation flow.
Originally released under working title ETISS-GTS.

<details>
<summary>ETISS-GTS Thesis</summary>
<p>

```
@mastersthesis{holappa2025etissgts,
  author = {Holappa, Heidi},
  title = {A Co-Verification Infrastructure for ETISS: Verifying Extended Instruction Sets},
  school = {University of Helsinki},
  year = {2025},
  url = {http://hdl.handle.net/10138/602402}
}
```

</p>
</details>

## Build the plugins

Build against an ETISS install or build tree that provides `ETISSConfig.cmake`:

```sh
cmake -S . -B build -D ETISS_DIR=/path/to/etiss/install/lib/CMake/ETISS
cmake --build build
cmake --install build --prefix /path/to/etiss/install
```

The plugin library is installed as `lib/plugins/libXValid.so` and registered in ETISS'
`lib/plugins/list.txt`.

The library provides these ETISS plugin names:

- `ISAExtensionValidator`
- `GTS`
- `DataWriteTracer`
- `DataReadTracer`

`GTS` expects the validation pipeline to provide `pcs.tmp` in the ETISS working directory and
writes binary trace data to `trace.bin`.

`DataWriteTracer` records traced memory writes as type-2 entries. `DataReadTracer` records
successful traced memory reads as type-3 entries after the underlying ETISS `dread` fills the
read buffer. Both plugins accept `plugin.data_*_tracer.logaddr`/`addr` and
`plugin.data_*_tracer.logmask`/`mask` options.

## Tracing with bare_etiss_processor

`bare_etiss_processor` does not call the XValid tracers directly. It loads them as normal ETISS
plugins from the ini file or command line, then `CPUCore::execute()` applies their
`SystemWrapperPlugin` hooks to the active `ETISS_System`.

The data tracers wrap the simulator memory callbacks:

- `DataWriteTracer` replaces `ETISS_System::dwrite`, records the PC, address, size, and write
  bytes while tracing is active, then forwards the write to the original memory system.
- `DataReadTracer` replaces `ETISS_System::dread`, forwards the read to the original memory
  system first, then records the PC, address, size, and returned bytes if the read succeeded.

`GTS`/`InstructionTracer` controls the trace window. It reads `pcs.tmp`, activates
`TraceFileWriter` when execution enters the selected PC range, emits type-1 CPU state snapshots,
and deactivates tracing at the matching return point. The data read/write tracers only emit
records while that writer is active, so load/store events in `trace.bin` are aligned with the
same function-level trace window.

Typical command-line loading looks like this:

```sh
bare_etiss_processor \
  -i path/to/program.ini \
  -p GTS DataWriteTracer DataReadTracer \
  --plugin.data_write_tracer.logaddr=0x0 \
  --plugin.data_write_tracer.logmask=0x0 \
  --plugin.data_read_tracer.logaddr=0x0 \
  --plugin.data_read_tracer.logmask=0x0
```

With address and mask set to zero, all reads or writes are traced while `GTS` has tracing active.
Use non-zero address/mask pairs to restrict tracing to a memory window, for example an MMIO region.

## Python validation pipeline

The validation pipeline is available as the root-level `py_etiss_xvalid` package:

```sh
python3 -m py_etiss_xvalid --help
```

Additional pipeline documentation lives in `docs/py_etiss_xvalid/README.md`.
