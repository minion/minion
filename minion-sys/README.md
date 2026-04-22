# `minion-sys`

This crate contains (in progress) Rust bindings for the [Minion](https://github.com/minion/minion) constraint solver.

Read the documentation [here](https://conjure-cp.github.io/conjure-oxide/docs/minion_sys/).

## Finding Minion at build time

`minion-sys` needs a Minion source tree to compile against. It locates one in this order:

1. `$MINION_SRC`, if set.
2. `./vendor/` — a bundled copy of Minion, used when this crate is packaged for release.
3. `../` — the parent directory, used when `minion-sys` lives inside a Minion checkout.

## Licence

This crate is licensed under the [Mozilla Public Licence 2.0](https://www.mozilla.org/en-US/MPL/2.0/).

## Debugging

Debug symbols for Minion can be enabled by setting the environment variable `DEBUG_MINION`.

Eg.

```shell
DEBUG_MINION=true cargo test
```
