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
