# `minion-sys` changelog

Minion's own release notes are in [`history.md`](https://github.com/minion/minion/blob/main/history.md)
at the top of the repository. This file covers the Rust crate, which is
versioned separately from the solver.

- Unreleased : 0.1.0

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
