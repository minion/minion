// adapted from
// - https://github.com/gokberkkocak/rust_glucose/blob/master/build.rs
// - https://rust-lang.github.io/rust-bindgen/non-system-libraries.html
// - https://doc.rust-lang.org/cargo/reference/build-scripts.html#rerun-if-changed #![allow(clippy::unwrap_used)]
#![allow(clippy::expect_used)]
#![allow(clippy::unwrap_used)]
#![allow(clippy::panic)]

use std::env;
use std::path::{Path, PathBuf};
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let minion_src = find_minion_src();

    println!("cargo:rustc-link-search=all={out_dir}/build");
    println!("cargo:rustc-link-lib=static=minion");
    println!("cargo:rerun-if-changed=vendor");
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=build.sh");
    println!("cargo:rerun-if-env-changed=DEBUG_MINION");
    println!("cargo:rerun-if-env-changed=MINION_SRC");
    println!(
        "cargo:rerun-if-changed={}/configure.py",
        minion_src.display()
    );
    println!("cargo:rerun-if-changed={}/minion", minion_src.display());

    build(&minion_src);

    // also need to (dynamically) link to c++ stdlib
    // https://flames-of-code.netlify.app/blog/rust-and-cmake-cplusplus/
    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=dylib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=dylib=stdc++");
    } else {
        unimplemented!();
    }

    bind(&minion_src);
}

/// Find the Minion source tree.
///
/// Preference order:
/// 1. `$MINION_SRC` if set (user override; must point at a valid tree).
/// 2. `./vendor/` — used when minion-sys is distributed with a bundled copy of Minion.
/// 3. `../` — used during development, when minion-sys lives inside the Minion repository.
fn find_minion_src() -> PathBuf {
    let crate_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());

    if let Ok(explicit) = env::var("MINION_SRC") {
        let p = PathBuf::from(&explicit);
        if is_minion_src(&p) {
            return p;
        }
        panic!(
            "minion-sys: MINION_SRC={explicit} does not look like a Minion source tree \
             (no configure.py or minion/ subdirectory)"
        );
    }

    let vendor = crate_dir.join("vendor");
    if is_minion_src(&vendor) {
        return vendor;
    }

    let parent = crate_dir.parent().unwrap().to_path_buf();
    if is_minion_src(&parent) {
        return parent;
    }

    panic!(
        "minion-sys: cannot locate Minion source tree. \
         Looked in {} and {}. \
         Either place a Minion checkout at minion-sys/vendor/, \
         run minion-sys from within the Minion repository, \
         or set MINION_SRC to point at a Minion checkout.",
        vendor.display(),
        parent.display()
    );
}

fn is_minion_src(p: &Path) -> bool {
    p.join("configure.py").is_file() && p.join("minion").is_dir()
}

fn build(minion_src: &Path) {
    let output = Command::new("bash")
        .args(["build.sh"])
        .env("MINION_SRC", minion_src)
        .output()
        .expect("Failed to run build.sh");

    /*
    do cargo build -vv to see
    */
    println!("stdout");
    println!("{}", String::from_utf8_lossy(&output.stdout));
    println!("stderr");
    println!("{}", String::from_utf8_lossy(&output.stderr));

    if !output.status.success() {
        panic!("build.sh has non zero exit status")
    }
}

fn bind(minion_src: &Path) {
    let out_dir = env::var("OUT_DIR").unwrap();
    let minion_inc = minion_src.join("minion");
    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    let bindings = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header(
            minion_inc
                .join("libwrapper.h")
                .to_str()
                .expect("minion src path must be UTF-8"),
        )
        // Make all templates opaque as reccomended by bindgen
        .opaque_type("std::.*")
        // Suppress layout tests - we use these structs only through opaque pointers
        .layout_tests(false)
        // Manually allow C++ functions to stop bindgen getting confused.
        .allowlist_function("minion_newContext")
        .allowlist_function("minion_freeContext")
        .allowlist_function("minion_activateContext")
        .allowlist_function("minion_deactivateContext")
        .allowlist_function("runMinion")
        .allowlist_function("runMinionParallel")
        .allowlist_function("runMinionWorkSteal")
        .allowlist_type("MinionThreadConfig")
        .allowlist_type("MinionWorkStealStats")
        .allowlist_function("minion_error_message")
        .allowlist_function("constantAsVar")
        .allowlist_function("tupleList_new")
        .allowlist_function("tupleList_free")
        .allowlist_function("minion_getVarByName")
        .allowlist_function("minion_newVar")
        .allowlist_function("minion_newSparseBoundVar")
        .allowlist_function("minion_addConstraintMidsearch")
        .allowlist_function("minion_newVarMidsearch")
        .allowlist_function("minion_newSparseBoundVarMidsearch")
        .allowlist_function("minion_getVarValue")
        .allowlist_function("instance_new")
        .allowlist_function("instance_free")
        .allowlist_function("instance_addSearchOrder")
        .allowlist_function("instance_addConstraint")
        .allowlist_function("instance_setOptimise")
        .allowlist_function("instance_addTupleTableSymbol")
        .allowlist_function("instance_getTupleTableSymbol")
        .allowlist_function("printMatrix_addVar")
        .allowlist_function("printMatrix_getValue")
        .allowlist_function("printMatrix_getValueByName")
        .allowlist_function("constraint_addList")
        .allowlist_function("constraint_new")
        .allowlist_function("constraint_free")
        .allowlist_function("constraint_addVar")
        .allowlist_function("constraint_addTwoVars")
        .allowlist_function("constraint_addConstant")
        .allowlist_function("constraint_addConstantList")
        .allowlist_function("constraint_addConstraint")
        .allowlist_function("constraint_addConstraintList")
        .allowlist_function("constraint_setTuples")
        .allowlist_function("constraint_setTuplesByName")
        .allowlist_function("searchOptions_new")
        .allowlist_function("searchOptions_free")
        .allowlist_function("searchMethod_new")
        .allowlist_function("searchMethod_free")
        .allowlist_function("searchOrder_new")
        .allowlist_function("searchOrder_free")
        .allowlist_function("searchOrder_setValOrder")
        .allowlist_function("vec_var_new")
        .allowlist_function("vec_var_push_back")
        .allowlist_function("vec_var_free")
        .allowlist_function("vec_int_new")
        .allowlist_function("vec_int_push_back")
        .allowlist_function("vec_int_free")
        .allowlist_function("vec_constraints_new")
        .allowlist_function("vec_constraints_push_back")
        .allowlist_function("vec_constraints_free")
        .allowlist_function("vec_vec_int_new")
        .allowlist_function("vec_vec_int_push_back")
        .allowlist_function("vec_vec_int_push_back_ptr")
        .allowlist_function("vec_vec_int_free")
        .allowlist_function("TableOut_get")
        .clang_arg(format!("-I{out_dir}/build/src/")) // generated from configure.py
        .clang_arg(format!("-I{}", minion_inc.display()))
        .clang_arg("-DLIBMINION")
        .clang_arg(r"--std=gnu++11")
        .clang_arg(r"-xc++");

    let bindings = if std::env::var("DEBUG_MINION").is_ok() {
        bindings
            .clang_arg("-g")
            .clang_arg("-D_GLIBCXX_DEBUG")
            .clang_arg("-DMORE_SEARCH_INFO")
            .clang_arg("-DMINION_DEBUG")
    } else {
        bindings
    };

    // Finish the builder and generate the bindings.
    let bindings = bindings
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings to file!");
}
