# `minion-sys` changelog

Minion's own release notes are in [`history.md`](https://github.com/minion/minion/blob/main/history.md)
at the top of the repository. This file covers the Rust crate, which is
versioned separately from the solver.

## 0.1.3 — 2026-11-18

- Fix printed variable declarations, true/false syntax, inline tuples, and
  watched constraint lists. Emit named tuple and short-tuple definitions across
  nested constraints, and print solution columns in declaration order.
- Fix parsing of empty inline tables and constraint lists, accept zero-arity
  tuples, and reject ragged tables whose first row is empty.
- Generate bindings from a lightweight library interface shared with Minion,
  keeping solver implementations out of bindgen's input. Move variable layouts,
  domain-width aliases, and search enums into canonical shared headers.
- Configure opaque search options and methods through setters instead of writing
  C++ object fields from Rust. This avoids the libc++ recursive-union panic with
  Emscripten 7 while continuing to use released bindgen 0.73.
- Include `<iterator>` explicitly for `std::insert_iterator`, fixing compilation
  with newer standard-library headers.

## 0.1.2 — 2026-10-05

- Reset Minion's wall-clock timers between runs, so a time limit from one run
  no longer fires during a later run in the same process.
- Stop generated headers from triggering repeated C++ rebuilds. Continue
  tracking source and system headers and bindgen environment overrides.
- Update `bindgen` to 0.73.

## 0.1.1 — 2026-09-18

- Support sequential `wasm32-unknown-emscripten` builds with automatic SDK
  discovery, Wasm exceptions, and checked binding generation. Reject unsupported
  parallel modes and time limits; mark unavailable CPU/RSS statistics explicitly.
- Reduce Wasm extendable storage reservations to avoid exhausting linear memory.
- Require a compatible host libclang for Emscripten bindings, with an early
  diagnostic for unsupported versions.
- Execute Wasm regression tests under Node in CI, including domains64 builds.

## 0.1.0

The first published release. `minion-sys` was written in
[conjure-oxide](https://github.com/conjure-cp/conjure-oxide), where it began in
October 2023 as `minion_rs`, and was imported into the Minion repository in
April 2026.

Until now it has only ever been built against a Minion checkout sitting around
it. It now embeds Minion's C++ source instead. The C interface `libminion`
exposes is not stable, so embedding is the only way to be sure the bindings and
the solver match.

- The build no longer shells out to `configure.py` and `make`. `build.rs` reads
  the same `/* JSON */` constraint declarations out of the headers, generates
  the same C++, and compiles it with the `cc` crate. CI diffs the two
  generators against each other so they cannot drift apart.
- Minion is now built optimised, whatever cargo profile you are in. The
  in-repository build was `--unoptimised`, so anything depending on
  `minion-sys` got a slow solver even in a release build. Assertions, which
  were on by default, are now behind the `dom-assert` feature.
- Minion is built with `QUICK_COMPILE`, which the `minion` binary is not. That
  keeps the static library at about 7 MB rather than 98 MB. It costs nothing on
  some models and about 3x on others -- sums over Booleans are the bad case --
  so `full-specialisation` turns it off. The README has the measurements.
- Build variants are cargo features: `full-specialisation`, `domains64`,
  `dom-assert`, `debug-minion`, `search-info`, `debug-print`, `no-wdeg` and
  `sanitize`. The `DEBUG_MINION` and `MINION_SANITIZE` environment variables
  still work.
- Fixed the crate failing to compile with 64-bit domains: the random seed was
  assigned as a fixed-width integer, and `DOMAINS64` widens it.
