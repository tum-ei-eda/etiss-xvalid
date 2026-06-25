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
- `DataWriteTracer`
- `DataReadTracer`

`ISAExtensionValidator` provides the trace window and state snapshot functionality. It writes binary trace data to
`trace.bin`, activates
`TraceFileWriter` inside the selected PC range, and emits type-1 CPU state snapshots at selected
instruction mnemonics. By default it instruments every decoded instruction. Use
`plugin.xvalid.itrace_instructions=name[,name...]` to select instruction types, or
`plugin.xvalid.itrace_instructions=*` to keep all instructions enabled.

By default `ISAExtensionValidator` reads `pcs.tmp` from the ETISS working directory. Set
`plugin.xvalid.itrace_pc_ranges_fpath` in the ETISS ini file to use an explicit path. For ad-hoc
runs, set `plugin.xvalid.itrace_pc_ranges` directly as `low:high[,low:high...]`. Numeric bounds may
be decimal or `0x`-prefixed hexadecimal, for example
`plugin.xvalid.itrace_pc_ranges=0x10000000:0x10000334,0x10001000:0x10001080`.

`DataWriteTracer` records traced memory writes as type-2 entries. `DataReadTracer` records
successful traced memory reads as type-3 entries after the underlying ETISS `dread` fills the
read buffer. The data tracers accept `plugin.xvalid.dwrite_trace.logaddr`/`logmask` and
`plugin.xvalid.dread_trace.logaddr`/`logmask` options.

## Tracing with bare_etiss_processor

`bare_etiss_processor` does not call the XValid tracers directly. It loads them as normal ETISS
plugins from the ini file or command line, then `CPUCore::execute()` applies their
`SystemWrapperPlugin` hooks to the active `ETISS_System`.

The data tracers wrap the simulator memory callbacks:

- `DataWriteTracer` replaces `ETISS_System::dwrite`, records the PC, address, size, and write
  bytes while tracing is active, then forwards the write to the original memory system.
- `DataReadTracer` replaces `ETISS_System::dread`, forwards the read to the original memory
  system first, then records the PC, address, size, and returned bytes if the read succeeded.

`ISAExtensionValidator` controls the trace window. It reads the configured PC range file or direct
range option, activates `TraceFileWriter` when execution enters the selected PC range, and emits
type-1 CPU state snapshots. The data read/write tracers only emit records while that writer is
active, so load/store events in `trace.bin` are aligned with the same function-level trace window.

Typical ini configuration looks like this:

```ini
[StringConfigurations]
plugin.xvalid.itrace_pc_ranges=0x10000000:0x10000334
plugin.xvalid.dwrite_trace.logaddr=0x0
plugin.xvalid.dwrite_trace.logmask=0x0
plugin.xvalid.dread_trace.logaddr=0x0
plugin.xvalid.dread_trace.logmask=0x0
```

Load the plugins with `bare_etiss_processor`:

```sh
bare_etiss_processor \
  -i path/to/program.ini \
  -p ISAExtensionValidator DataWriteTracer DataReadTracer
```

With address and mask set to zero, all reads or writes are traced while `ISAExtensionValidator`
has tracing active. Use non-zero address/mask pairs to restrict tracing to a memory window, for
example an MMIO region.

## Python validation pipeline

The validation pipeline is available as the root-level `py_etiss_xvalid` package:

```sh
python3 -m py_etiss_xvalid --help
```

Additional pipeline documentation lives in `docs/py_etiss_xvalid/README.md`.
