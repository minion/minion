//! Builds Minion and generates the Rust bindings to it.
//!
//! Minion's own build (`configure.py`) generates C++ from a constraint registry
//! before compiling: each constraint is declared by a `/* JSON ... */` block in
//! its header, and those blocks drive `ConstraintEnum.h`, `constraint_defs.h`
//! and the `build_constraint` dispatch. This file reimplements that generation
//! step so the crate builds with nothing but a C++ compiler -- no python3, no
//! make, no bash. `generate_sources` below must stay faithful to
//! `configure.py`; the `generator-parity` CI job diffs the two.

#![allow(clippy::expect_used)]
#![allow(clippy::unwrap_used)]
#![allow(clippy::panic)]

use std::collections::BTreeSet;
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

/// The Minion sources compiled into `libminion`, relative to the Minion root.
///
/// Mirrors `minionlibsrclist` in `configure.py`. `minion/main.cpp` is
/// deliberately absent: it is the `minion` binary's entry point, and linking it
/// into the library would give us a second `main`.
const LIB_SOURCES: &[&str] = &[
    "minion/BuildVariables.cpp",
    "minion/BuildCSP.cpp",
    "minion/buildConstraints.cpp",
    "minion/commandline_parse.cpp",
    "minion/debug_functions.cpp",
    "minion/get_info.cpp",
    "minion/info_dumps.cpp",
    "minion/minion.cpp",
    "minion/globals.cpp",
    "minion/preprocess.cpp",
    "minion/system/trigger_timer.cpp",
    "minion/system/sha1.cpp",
    "minion/help/help.cpp",
    "minion/inputfile_parse/inputfile_parse.cpp",
    "minion/dump_state.cpp",
    "minion/parallel.cpp",
    "minion/parallel/preprocess_parallel.cpp",
    "minion/parallel/work_steal.cpp",
    "minion/search_dump.cpp",
    "minion/command_search.cpp",
    "minion/libwrapper.cpp",
];

/// The argument kinds a constraint's JSON block may name.
///
/// `configure.py` does not check these, but an unrecognised kind means the
/// registry has grown a feature this generator does not know about, and
/// silently emitting it would produce a `constraint_defs.h` that does not
/// compile -- or worse, one that does. Fail loudly instead.
const ARG_KINDS: &[&str] = &[
    "read_var",
    "read_list",
    "read_constant",
    "read_constant_list",
    "read_tuples",
    "read_2_vars",
    "read_constraint",
    "read_short_tuples",
    "read_constraint_list",
];

/// Argument kinds that contribute to a constraint's variable count, as counted
/// by `configure.py`'s `varcount`.
const VAR_KINDS: &[&str] = &["read_list", "read_var", "read_2_vars"];

/// One constraint, as declared by a `/* JSON ... */` block in a Minion header.
///
/// Field order is the sort order: `configure.py` sorts with
/// `sorted(d.items())`, which compares the dict's items sorted by key, i.e.
/// `args`, then `filename`, then `internal_name`, `name`, `type`.
#[derive(PartialEq, Eq, PartialOrd, Ord)]
struct Constraint {
    args: Vec<String>,
    /// Path to the declaring header, relative to the Minion root.
    filename: String,
    internal_name: String,
    name: String,
    kind: String,
}

impl Constraint {
    /// The number of arguments that are variables, for `BUILD_CT`.
    fn var_count(&self) -> usize {
        self.args
            .iter()
            .filter(|a| VAR_KINDS.contains(&a.as_str()))
            .count()
    }

    /// The declaring header, relative to `minion/` -- the form we `#include`.
    ///
    /// `configure.py` emits a path relative to the build directory instead,
    /// which needs `os.path.relpath` and breaks across Windows drive letters.
    /// Both resolve to the same header: `-I <minion>` is on the command line
    /// already, for `#include "minion.h"`.
    fn include_path(&self) -> &str {
        self.filename
            .strip_prefix("minion/")
            .expect("constraint headers live under minion/")
    }
}

fn main() {
    let minion_src = find_minion_src();
    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR is set by cargo"));
    let gen_dir = out_dir.join("minion_generated");

    // Emitted before any real work, so a build script that fails still leaves
    // cargo with correct dirtiness information. Note there is deliberately no
    // `rerun-if-changed=vendor`: in a development checkout vendor/ does not
    // exist, and cargo treats a declared path that is missing as dirty, which
    // would rebuild the world on every single `cargo build`.
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed={}", minion_src.join("minion").display());
    for var in [
        "MINION_SRC",
        "DEBUG_MINION",
        "MINION_SANITIZE",
        "MINION_OPT_LEVEL",
        "MINION_GIT_VER",
    ] {
        println!("cargo:rerun-if-env-changed={var}");
    }

    let config = Config::detect();
    // Visible with `cargo build -vv`, and in the build script's `output` file.
    // Worth having: the defines decide what the solver actually does, and they
    // come from a mix of cargo features and environment variables.
    println!(
        "minion-sys: source {}, defines {}",
        minion_src.display(),
        config.defines.join(" ")
    );

    let constraints = collect_constraints(&minion_src);
    let generated = generate_sources(&constraints, &minion_src, &gen_dir);

    // Bindgen first: it takes seconds where cc takes minutes, so a bad header
    // fails fast rather than after a full C++ build.
    generate_bindings(&minion_src, &gen_dir, &config);
    compile(&minion_src, &gen_dir, &generated, &config);
}

/// Find the Minion source tree.
///
/// 1. `$MINION_SRC` if set (user override; must point at a valid tree).
/// 2. `./vendor/` -- the copy bundled into the published crate.
/// 3. `../` -- used during development, when minion-sys lives inside the
///    Minion repository.
fn find_minion_src() -> PathBuf {
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("set by cargo"));

    if let Ok(explicit) = env::var("MINION_SRC") {
        let p = PathBuf::from(&explicit);
        assert!(
            is_minion_src(&p),
            "minion-sys: MINION_SRC={explicit} does not look like a Minion source tree \
             (no minion/libwrapper.h underneath it)"
        );
        return p;
    }

    let vendor = crate_dir.join("vendor");
    if is_minion_src(&vendor) {
        return vendor;
    }

    let parent = crate_dir.parent().expect("crate dir has a parent").to_path_buf();
    if is_minion_src(&parent) {
        return parent;
    }

    panic!(
        "minion-sys: cannot locate Minion source tree. Looked in {} and {}. \
         Either place a Minion checkout at minion-sys/vendor/, run minion-sys from \
         within the Minion repository, or set MINION_SRC to point at a Minion checkout.",
        vendor.display(),
        parent.display()
    );
}

fn is_minion_src(p: &Path) -> bool {
    p.join("minion").join("libwrapper.h").is_file()
}

/// Read every constraint declaration out of the Minion headers.
///
/// Follows `configure.py`: walk `minion/` for `.h` and `.hpp` files, pull out
/// each `/* JSON ... */` comment block, and parse it. Anything unexpected --
/// malformed JSON, an unknown key, a duplicate name -- is a hard error. A
/// constraint quietly dropped here is a propagator quietly missing from the
/// solver, which is the one failure mode this whole file exists to avoid.
fn collect_constraints(minion_src: &Path) -> Vec<Constraint> {
    let minion_dir = minion_src.join("minion");
    let mut headers = Vec::new();
    collect_headers(&minion_dir, &mut headers);
    assert!(!headers.is_empty(), "no headers found under {}", minion_dir.display());

    let mut constraints = Vec::new();
    for header in headers {
        let relative = header
            .strip_prefix(minion_src)
            .expect("header is under the minion source root")
            .to_str()
            .expect("minion paths are UTF-8")
            .replace('\\', "/");
        let text = fs::read_to_string(&header)
            .unwrap_or_else(|e| panic!("cannot read {}: {e}", header.display()));
        for block in json_blocks(&text, &header) {
            constraints.push(parse_constraint(&block, &relative, &header));
        }
    }

    validate(&constraints);
    constraints.sort();
    constraints
}

fn collect_headers(dir: &Path, out: &mut Vec<PathBuf>) {
    let mut entries: Vec<PathBuf> = fs::read_dir(dir)
        .unwrap_or_else(|e| panic!("cannot read directory {}: {e}", dir.display()))
        .map(|e| e.expect("directory entry").path())
        .collect();
    entries.sort();

    for entry in entries {
        if entry.is_dir() {
            collect_headers(&entry, out);
        } else if matches!(
            entry.extension().and_then(|e| e.to_str()),
            Some("h") | Some("hpp")
        ) {
            out.push(entry);
        }
    }
}

/// Extract the text of each `/* JSON ... */` block, as `configure.py` does:
/// everything between the marker and the next `*/`.
fn json_blocks(text: &str, path: &Path) -> Vec<String> {
    const MARKER: &str = "/* JSON";
    let mut blocks = Vec::new();
    let mut rest = text;

    while let Some(start) = rest.find(MARKER) {
        let body_start = start + MARKER.len();
        let end = rest[body_start..].find("*/").unwrap_or_else(|| {
            panic!(
                "found the start of a JSON comment but not its end in {}",
                path.display()
            )
        }) + body_start;
        blocks.push(rest[body_start..end].to_string());
        rest = &rest[end..];
    }

    blocks
}

fn parse_constraint(block: &str, relative: &str, path: &Path) -> Constraint {
    let value: serde_json::Value = serde_json::from_str(block).unwrap_or_else(|e| {
        panic!("invalid JSON in {}: {e}\n{block}", path.display())
    });
    let object = value
        .as_object()
        .unwrap_or_else(|| panic!("JSON block in {} is not an object", path.display()));

    for key in object.keys() {
        assert!(
            matches!(key.as_str(), "type" | "name" | "internal_name" | "args"),
            "unknown key {key:?} in the JSON block in {}",
            path.display()
        );
    }

    let string = |key: &str| -> String {
        object
            .get(key)
            .and_then(|v| v.as_str())
            .unwrap_or_else(|| {
                panic!("JSON block in {} has no string {key:?}", path.display())
            })
            .to_string()
    };

    let kind = string("type");
    assert_eq!(
        kind,
        "constraint",
        "bad 'type' in the JSON block in {}",
        path.display()
    );

    let args: Vec<String> = object
        .get("args")
        .and_then(|v| v.as_array())
        .unwrap_or_else(|| panic!("JSON block in {} has no 'args' array", path.display()))
        .iter()
        .map(|a| {
            let arg = a
                .as_str()
                .unwrap_or_else(|| panic!("non-string arg in {}", path.display()))
                .to_string();
            assert!(
                ARG_KINDS.contains(&arg.as_str()),
                "unknown argument kind {arg:?} in {}",
                path.display()
            );
            arg
        })
        .collect();

    Constraint {
        args,
        filename: relative.to_string(),
        internal_name: string("internal_name"),
        name: string("name"),
        kind,
    }
}

/// Reject duplicate names, as `configure.py`'s `validate_names` does. Two
/// constraints sharing an `internal_name` would collide in `ConstraintType`;
/// two sharing a `name` would make one of them unreachable from the parser.
fn validate(constraints: &[Constraint]) {
    assert!(!constraints.is_empty(), "found no constraints in the Minion source");

    let mut names = BTreeSet::new();
    let mut internal_names = BTreeSet::new();
    for c in constraints {
        assert!(
            internal_names.insert(&c.internal_name),
            "duplicate internal_name: {}",
            c.internal_name
        );
        assert!(names.insert(&c.name), "duplicate name: {}", c.name);

        // constraint_defs.h puts the name straight into a C string literal,
        // and ConstraintDef::read_types is a std::array<ReadTypes, 5>.
        assert!(
            !c.name.contains('"') && !c.name.contains('\\') && c.name.is_ascii(),
            "constraint name {:?} cannot be written as a C string literal",
            c.name
        );
        assert!(
            c.args.len() <= 5,
            "{} has {} arguments; ConstraintDef holds at most 5",
            c.internal_name,
            c.args.len()
        );
        // build_helper.h only defines TERMINATE_BUILDCON0..4.
        assert!(
            c.var_count() <= 4,
            "{} has {} variable arguments; BUILD_CT supports at most 4",
            c.internal_name,
            c.var_count()
        );
    }
}

/// Write the generated C++ into `gen_dir`, returning the sources to compile.
///
/// Faithful to `configure.py` lines 304-366. The one intentional difference is
/// the `#include` of the declaring header -- see `Constraint::include_path`.
fn generate_sources(
    constraints: &[Constraint],
    minion_src: &Path,
    gen_dir: &Path,
) -> Vec<PathBuf> {
    fs::create_dir_all(gen_dir)
        .unwrap_or_else(|e| panic!("cannot create {}: {e}", gen_dir.display()));

    let mut sources = Vec::new();

    // Constraints, five to a translation unit. Splitting them keeps any one
    // unit's compile time and memory use bounded; the templates here are heavy.
    for (index, chunk) in constraints.chunks(5).enumerate() {
        let mut body = String::from("// Minion constraint file\n");
        for c in chunk {
            body.push_str("#include \"minion.h\"\n");
            body.push_str(&format!("#include \"{}\"\n\n", c.include_path()));
            body.push_str(&format!(
                "BUILD_CT({}, {})\n",
                c.internal_name,
                c.var_count()
            ));
        }
        sources.push(write(gen_dir, &format!("build_constraint_{}.cpp", index + 1), &body));
    }

    // The dispatch from a parsed constraint back to its builder.
    let mut start = String::from("#include \"minion.h\"\n");
    for c in constraints {
        start.push_str(&format!(
            "AbstractConstraint* build_constraint_{}(ConstraintBlob&);\n",
            c.internal_name
        ));
    }
    start.push_str("AbstractConstraint* build_constraint(ConstraintBlob& b) {\n");
    start.push_str("  switch(b.constraint->type) {\n");
    for c in constraints {
        start.push_str(&format!("  case {}:\n", c.internal_name));
        start.push_str(&format!(
            "    return build_constraint_{}(b);\n",
            c.internal_name
        ));
    }
    start.push_str("  default: abort();\n");
    start.push_str("  }\n}\n");
    sources.push(write(gen_dir, "BuildStaticStart.cpp", &start));

    // The table the input parser searches by name, included by globals.cpp.
    let mut defs = String::from("ConstraintDef constraint_list[] = {\n");
    for c in constraints {
        defs.push_str(&format!(
            "{{ \"{}\", {}, {}, {{ {{{}}} }} }},\n",
            c.name,
            c.internal_name,
            c.args.len(),
            c.args.join(",")
        ));
    }
    defs.push_str("};\n");
    write(gen_dir, "constraint_defs.h", &defs);

    // The enum, included by libwrapper.h -- and so read by bindgen, which is
    // why generation has to happen before both cc and bindgen.
    let mut enumeration = String::from("#ifndef CONSTRAINT_ENUM_QWE\n#define CONSTRAINT_ENUM_QWE\nenum ConstraintType {\n");
    for c in constraints {
        enumeration.push_str(&format!("{},\n", c.internal_name));
    }
    enumeration.push_str("};\n#endif\n");
    write(gen_dir, "ConstraintEnum.h", &enumeration);

    write(
        gen_dir,
        "BuildDefines.h",
        &format!("#define GIT_VER {}\n", git_version(minion_src)),
    );

    sources
}

fn write(dir: &Path, name: &str, contents: &str) -> PathBuf {
    let path = dir.join(name);
    fs::write(&path, contents)
        .unwrap_or_else(|e| panic!("cannot write {}: {e}", path.display()));
    path
}

/// The version string Minion reports, as a quoted C string literal.
///
/// `GIT_VER` is only printed and written to `-tableout`; nothing branches on
/// it. The published crate carries no git repository, so the vendoring script
/// records the revision in `vendor/GIT_VERSION` and we use that. In a
/// development checkout we ask git directly, matching `configure.py`.
fn git_version(minion_src: &Path) -> String {
    if let Ok(explicit) = env::var("MINION_GIT_VER") {
        return format!("{:?}", explicit.trim());
    }

    let recorded = minion_src.join("GIT_VERSION");
    if let Ok(text) = fs::read_to_string(&recorded) {
        return format!("{:?}", text.trim());
    }

    let described = std::process::Command::new("git")
        .args(["-C"])
        .arg(minion_src)
        .args(["log", "-1", "--pretty=format:%h (%ai)"])
        .output()
        .ok()
        .filter(|o| o.status.success())
        .and_then(|o| String::from_utf8(o.stdout).ok());

    match described {
        Some(v) if !v.trim().is_empty() => format!("{:?}", v.trim()),
        _ => format!("\"minion-sys {}\"", env!("CARGO_PKG_VERSION")),
    }
}

/// Is a cargo feature enabled? Build scripts are compiled without the crate's
/// features, so `cfg!(feature = ...)` is always false here; cargo passes them
/// in the environment instead.
fn feature(name: &str) -> bool {
    env::var_os(format!("CARGO_FEATURE_{name}")).is_some()
}

/// How this build of Minion is configured.
struct Config {
    /// Preprocessor defines, used for *both* the C++ compilation and bindgen.
    ///
    /// The two must agree. `DOMAINS64` widens `DomainInt`, and `DomainInt`
    /// crosses the FFI by value -- so a define applied to one side and not the
    /// other is not a link error, it is silent memory corruption. Computing
    /// them once here is what stops that happening.
    defines: Vec<String>,
    sanitize: bool,
    debug: bool,
}

impl Config {
    fn detect() -> Config {
        // The env vars predate the cargo features and are still used by
        // test.sh, CI.yml and mini-scripts/soak-debug.sh. Keep both working.
        let sanitize = feature("SANITIZE") || env::var_os("MINION_SANITIZE").is_some();
        let debug = feature("DEBUG_MINION") || env::var_os("DEBUG_MINION").is_some();

        let mut defines = vec!["LIBMINION".to_string()];

        if !feature("NO_WDEG") {
            defines.push("WDEG".to_string());
        }

        // QUICK_COMPILE drops the per-variable-type specialisations in
        // BuildConstraintConstructs.h, so every constraint goes through the
        // generic AnyVarRef path. It is on by default: without it the static
        // library is 98MB rather than 6.7MB. Cargo features are additive, so
        // "on unless you say otherwise" has to be spelled as the opposite
        // feature.
        if !feature("FULL_SPECIALISATION") {
            defines.push("QUICK_COMPILE".to_string());
        }
        if feature("DOMAINS64") {
            defines.push("DOMAINS64".to_string());
        }
        if feature("SEARCH_INFO") {
            defines.push("MORE_SEARCH_INFO".to_string());
        }
        if feature("DEBUG_PRINT") {
            defines.push("MINION_DEBUG_PRINT".to_string());
        }
        // MINION_DEBUG implies DOM_ASSERT in system.h, so `debug` subsumes it.
        if feature("DOM_ASSERT") || sanitize {
            defines.push("DOM_ASSERT".to_string());
        }
        if debug {
            defines.push("_GLIBCXX_DEBUG".to_string());
            defines.push("MINION_DEBUG".to_string());
            defines.push("MORE_SEARCH_INFO".to_string());
        }

        defines.sort();
        defines.dedup();
        Config { defines, sanitize, debug }
    }
}

/// Compile Minion into a static library and tell cargo to link it.
///
/// The flags mirror `configure.py`, minus its `ccache`/`sccache`/`gold`
/// probing of `$PATH`: that makes a build depend on what happens to be
/// installed. `cc` reads `CXX` and understands compiler wrappers, so
/// `CXX="ccache c++"` gives the same benefit without the guessing.
///
/// The optimisation level is fixed rather than following cargo's profile. A
/// `cargo build` in the dev profile still wants a solver that solves, and an
/// unoptimised Minion costs far more than a debug build of the calling crate
/// saves. `MINION_OPT_LEVEL` overrides it.
fn compile(minion_src: &Path, gen_dir: &Path, generated: &[PathBuf], config: &Config) {
    let target = env::var("TARGET").expect("TARGET is set by cargo");
    let msvc = target.contains("msvc");

    let mut build = cc::Build::new();
    build
        .cpp(true)
        .include(minion_src.join("minion"))
        .include(gen_dir)
        // cc otherwise scrapes cargo's DEBUG, and -g over 39 heavy C++
        // translation units is a lot of debug info nobody asked for.
        .debug(config.debug || config.sanitize);

    for define in &config.defines {
        build.define(define, None);
    }

    if msvc {
        build.warnings(false);
    } else {
        build
            .std("gnu++14")
            .warnings(true)
            .extra_warnings(true)
            .flag("-Wno-unused-parameter")
            .flag("-Wno-sign-compare")
            .flag("-pthread");
    }

    for source in LIB_SOURCES {
        let path = minion_src.join(source);
        assert!(path.is_file(), "missing Minion source file: {}", path.display());
        build.file(path);
    }
    for source in generated {
        build.file(source);
    }

    if config.sanitize {
        // AddressSanitizer build. The final Rust link must add the ASan
        // runtime itself, e.g.:
        //   RD=$(clang++ -print-resource-dir)/lib/darwin
        //   cargo rustc --release --bin tester --target-dir target-asan -- \
        //     -C link-arg=-fsanitize=address -C link-arg=-L$RD \
        //     -C link-arg=-lclang_rt.asan_osx_dynamic -C link-arg=-Wl,-rpath,$RD
        // (rustc links with -nodefaultlibs, so clang won't add it.)
        build.compiler("clang++").opt_level(0).flag("-fsanitize=address");
    } else {
        let level = env::var("MINION_OPT_LEVEL").unwrap_or_else(|_| "3".to_string());
        build.opt_level_str(&level);
        if !msvc {
            build.flag_if_supported("-fomit-frame-pointer");
        }
    }

    // Emits cargo:rustc-link-lib=static=minion, the link search path, and the
    // right C++ standard library for the target -- in that order, which is the
    // order a static archive needs.
    build.compile("minion");

    // Rust's std already pulls this in on Linux, but the archive genuinely
    // depends on it and saying so is free.
    if target.contains("linux") {
        println!("cargo:rustc-link-lib=dylib=pthread");
    }
}

/// Generate the Rust declarations for Minion's C interface.
///
/// This runs against the freshly generated `ConstraintEnum.h` (included by
/// `libwrapper.h`) and `BuildDefines.h` (included by `minion.h`). Checking
/// `bindings.rs` into the repository instead would let the Rust view of
/// `ConstraintType` drift from the C++ one, and a constraint built under the
/// wrong enum value is a wrong answer with no error -- so the bindings are
/// always generated from the sources we are about to compile.
fn generate_bindings(minion_src: &Path, gen_dir: &Path, config: &Config) {
    let out_dir = PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR is set by cargo"));
    let minion_inc = minion_src.join("minion");
    let header = minion_inc.join("libwrapper.h");

    let mut builder = bindgen::Builder::default()
        .header(header.to_str().expect("minion src path must be UTF-8"))
        // Make all templates opaque, as recommended by bindgen.
        .opaque_type("std::.*")
        // We use these structs only through opaque pointers.
        .layout_tests(false)
        .clang_arg(format!("-I{}", gen_dir.display()))
        .clang_arg(format!("-I{}", minion_inc.display()))
        .clang_arg("-std=gnu++14")
        .clang_arg("-xc++");

    for define in &config.defines {
        builder = builder.clang_arg(format!("-D{define}"));
    }

    // Named explicitly, rather than by pattern, to stop bindgen wandering into
    // the C++ headers libwrapper.h pulls in.
    for function in ALLOWED_FUNCTIONS {
        builder = builder.allowlist_function(function);
    }
    for ty in ["MinionThreadConfig", "MinionWorkStealStats"] {
        builder = builder.allowlist_type(ty);
    }

    builder
        .generate()
        .expect("unable to generate bindings")
        .write_to_file(out_dir.join("bindings.rs"))
        .expect("couldn't write bindings to file");
}

const ALLOWED_FUNCTIONS: &[&str] = &[
    "minion_newContext",
    "minion_freeContext",
    "minion_activateContext",
    "minion_deactivateContext",
    "runMinion",
    "runMinionParallel",
    "runMinionWorkSteal",
    "minion_error_message",
    "constantAsVar",
    "tupleList_new",
    "tupleList_free",
    "shortTupleList_new",
    "shortTupleList_free",
    "minion_getVarByName",
    "minion_newVar",
    "minion_newSparseBoundVar",
    "minion_addConstraintMidsearch",
    "minion_newVarMidsearch",
    "minion_newSparseBoundVarMidsearch",
    "minion_getVarValue",
    "instance_new",
    "instance_free",
    "instance_addSearchOrder",
    "instance_addConstraint",
    "instance_setOptimise",
    "instance_addTupleTableSymbol",
    "instance_getTupleTableSymbol",
    "instance_addShortTupleTableSymbol",
    "instance_getShortTupleTableSymbol",
    "printMatrix_addVar",
    "printMatrix_getValue",
    "printMatrix_getValueByName",
    "constraint_addList",
    "constraint_new",
    "constraint_free",
    "constraint_addVar",
    "constraint_addTwoVars",
    "constraint_addConstant",
    "constraint_addConstantList",
    "constraint_addConstraint",
    "constraint_addConstraintList",
    "constraint_setTuples",
    "constraint_setTuplesByName",
    "constraint_setShortTuples",
    "constraint_setShortTuplesByName",
    "searchOptions_new",
    "searchOptions_free",
    "searchMethod_new",
    "searchMethod_free",
    "searchOrder_new",
    "searchOrder_free",
    "searchOrder_setValOrder",
    "vec_var_new",
    "vec_var_push_back",
    "vec_var_free",
    "vec_int_new",
    "vec_int_push_back",
    "vec_int_free",
    "vec_constraints_new",
    "vec_constraints_push_back",
    "vec_constraints_free",
    "vec_vec_int_new",
    "vec_vec_int_push_back",
    "vec_vec_int_push_back_ptr",
    "vec_vec_int_free",
    "TableOut_get",
];
