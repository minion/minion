# `minion-sys`

Rust bindings to the [Minion](https://github.com/minion/minion) constraint
solver.

Minion's C++ source is embedded in this crate, so building it needs nothing but
a C++ compiler. `libminion`'s C interface is not stable, so embedding is the
only way to be sure the bindings and the solver match.

```shell
cargo add minion-sys
```

```rust
use minion_sys::ast::*;
use minion_sys::run_minion;
use std::collections::HashMap;

let mut model = Model::new();
model.named_variables.add_var("x".to_owned(), VarDomain::Bound(1, 3));
model.named_variables.add_var("y".to_owned(), VarDomain::Bound(2, 4));
model.constraints.push(Constraint::SumLeq(
    vec![Var::NameRef("x".to_owned()), Var::NameRef("y".to_owned())],
    Var::ConstantAsVar(4),
));

let mut solutions: Vec<HashMap<VarName, Constant>> = vec![];
run_minion(model, Box::new(|sol| { solutions.push(sol); true })).unwrap();
```

The crate documentation has the full API, including mid-search variable and
constraint injection, optimisation, and the parallel and work-stealing search
modes.

## Building

You need a C++14 compiler and `libclang` (for
[bindgen](https://rust-lang.github.io/rust-bindgen/requirements.html)).

Compilation goes through the [`cc`](https://docs.rs/cc) crate, so `CXX`,
`CXXFLAGS` and the usual cross-compilation variables all apply.

### Emscripten WebAssembly

`wasm32-unknown-emscripten` is detected automatically. This supports one
sequential solver per Web Worker with unshared memory, without pthread pools
or cross-origin isolation. It is not a `wasm32-unknown-unknown` port.
The supported/tested toolchain is Rust **1.98.0**, Emscripten **6.0.9**, and
Node **24 or newer**. Install that Rust target and activate the SDK so `em++`,
`em-config` and `emar` are on PATH. Bindgen needs **host libclang 20 or newer**
to parse this SDK's libc++ headers; Ubuntu 24.04's default `libclang-dev` is
version 18 and is too old. CI installs Clang 22 from
[LLVM's apt repository](https://apt.llvm.org/) and sets
`LIBCLANG_PATH=/usr/lib/llvm-22/lib` and `CLANG_PATH=/usr/bin/clang-22` so bindgen
uses the matching library and header-discovery driver. Installing Emscripten
alone does not upgrade the host libclang used by bindgen.

Bindgen discovers the SDK with `em-config CACHE` and uses its sysroot, libc++
and compatibility headers automatically. `MINION_EM_CONFIG` can name an
alternative executable; `MINION_EMSCRIPTEN_SYSROOT` can override the sysroot.
The build tracks these and the SDK environment variables. The C++ defines,
including `domains64`, are shared with bindgen; missing functions or constraint
enum variants fail the build.

The final executable must select libc++ and native Wasm exception handling:

```sh
export CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS="-C link-arg=-sDEFAULT_TO_CXX=1 -C link-arg=-fwasm-exceptions -C link-arg=-sALLOW_MEMORY_GROWTH=1 -C link-arg=-sSTACK_SIZE=8388608"
cargo +1.98.0 build --target wasm32-unknown-emscripten
```

These are application link settings: Cargo does not propagate a dependency's
`rustc-link-arg` to its consumers. C++ compilation uses `-fwasm-exceptions`
automatically. Do not add `-fexceptions` or `-sDISABLE_EXCEPTION_CATCHING=0`:
those select the incompatible JavaScript exception mode. See
[Emscripten's exception documentation](https://emscripten.org/docs/porting/exceptions.html).

Backtracking initially allocates **64 MiB**; enable memory growth as above or
provide enough initial memory for that block plus the model and runtime.
The tested stack size is **8 MiB**. Ordinary backtracking blocks are unchanged.
Extendable variable-storage blocks reserve **16 MiB each** on Emscripten instead
of the native 512 MiB: Wasm commits these reservations rather than reserving
virtual address space. Exceeding this fixed capacity returns a memory error;
blocks never move, preserving propagators' pointers.

Portfolio/thread/work-stealing APIs (even with one worker), process-based
preprocessing, and CPU/wall time limits return recoverable errors. Node limits
and callback early stop work. Cancel or enforce deadlines by terminating the
host Web Worker. CPU and RSS table fields (`PreprocessTime`, `SolveTime`,
`TotalTime`, `TotalSystemTime`, `MaxRSSkB`) report `unavailable`; `TotalWallTime`
remains numeric. Consumers must not parse unavailable statistics as numbers.
Sanitizer builds are not supported for this target.

From the Minion checkout, run the linked Node regression suite with:

```sh
bash mini-scripts/test-emscripten.sh --features dom-assert
bash mini-scripts/test-emscripten.sh --features dom-assert,domains64
```

This checks unshared memory, GCC enumeration, unsatisfiable models, C++ exception
recovery, early stop, repeated context destruction, unsupported modes, node
limits, and the existing mid-search backtracking regressions. Browser execution
is not covered by this suite.

Conjure Oxide's experimental Minion build can use this checkout by setting
`MINION_SYS_PATH=/path/to/minion/minion-sys` and `CONJURE_WEB_SOLVER=minion`
when running its `tools/build-essence-web.sh`; no bindgen header overrides or
local minion-sys patch are needed.

### Features

| Feature | Effect |
| --- | --- |
| `full-specialisation` | Restore Minion's per-variable-type specialisations. Solves faster; the static library grows from about 7&nbsp;MB to about 98&nbsp;MB, and takes far longer to compile. |
| `domains64` | 64-bit domains. Widens `SysInt`, which crosses the FFI. |
| `dom-assert` | Turns on fast debug checks. |
| `debug-minion` | Turns on more debug checks. |
| `search-info` | Extra search statistics. |
| `debug-print` | Verbose propagation tracing. |
| `no-wdeg` | Drop the wdeg heuristics. |
| `sanitize` | Build Minion under AddressSanitizer. |

By default Minion is compiled with `QUICK_COMPILE`, which massively reduces
compile time and size at the cost of being slower, particularly on problems
with many booleans. Measured over 300,000 search nodes, so both builds do
exactly the same search:

| Instance | `QUICK_COMPILE` | full | ratio |
| --- | --- | --- | --- |
| `benchmarks/Bibd/bibdline11` | 7.8&nbsp;s | 2.4&nbsp;s | 3.2x |
| `benchmarks/graceful/k7p2_table` | 33.8&nbsp;s | 32.7&nbsp;s | 1.03x |

### Environment variables

| Variable | Effect |
| --- | --- |
| `MINION_SRC` | Build against a Minion checkout instead of the bundled copy. |
| `MINION_OPT_LEVEL` | Optimisation level for the C++ (default `3`). |
| `MINION_GIT_VER` | The version string Minion reports. |
| `DEBUG_MINION` | As the `debug-minion` feature. |
| `MINION_SANITIZE` | As the `sanitize` feature. |

An AddressSanitizer build needs the final Rust link to add the ASan runtime
itself, because rustc links with `-nodefaultlibs` and so clang will not add it:

```shell
RD=$(clang++ -print-resource-dir)/lib/darwin
MINION_SANITIZE=1 cargo rustc --release --target-dir target-asan -- \
  -C link-arg=-fsanitize=address -C link-arg=-L$RD \
  -C link-arg=-lclang_rt.asan_osx_dynamic -C link-arg=-Wl,-rpath,$RD
```

### Where the Minion source comes from

1. `$MINION_SRC`, if set.
2. `vendor/`, the copy bundled into the published crate.
3. `../`, when `minion-sys` is being built inside a Minion checkout.

## Origin

Most of this crate was written in
[conjure-oxide](https://github.com/conjure-cp/conjure-oxide), starting in
October 2023 as `minion_rs`, and was imported here in April 2026.

Niklas Dewally and Vlad Tronciu wrote the original crate, with further
contributions from Özgür Akgün, Felix Leitner and Georgii Skorokhod. Chris
Jefferson maintains it here.

## Changes

[`CHANGELOG.md`](CHANGELOG.md) covers the crate. Minion's own release notes are
in `history.md` at the top of the repository.

## Licence

[Mozilla Public Licence 2.0](https://www.mozilla.org/en-US/MPL/2.0/), the same
as Minion itself.
