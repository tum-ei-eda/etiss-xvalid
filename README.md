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

`GTS` expects the validation pipeline to provide `pcs.tmp` in the ETISS working directory and
writes binary trace data to `trace.bin`.

## Python validation pipeline

The validation pipeline is available as the root-level `py_etiss_xvalid` package:

```sh
python3 -m py_etiss_xvalid --help
```

Additional pipeline documentation lives in `docs/py_etiss_xvalid/README.md`.
