# ETISS XValid

Out-of-tree ETISS plugins and Python tooling for the XValid validation flow.

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
