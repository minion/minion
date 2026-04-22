//! Error types.

use thiserror::Error;

use crate::ffi;

/// Wraps all error types returned by `minion-sys`.
#[derive(Debug, Error)]
#[non_exhaustive]
pub enum MinionError {
    /// An error has occurred during the execution of Minion.
    #[error("runtime error: `{0}.to_string()`")]
    RuntimeError(#[from] RuntimeError),

    /// The input model uses Minion features that are not yet implemented in `minion_rs`.
    #[error("not implemented: {0}")]
    NotImplemented(String),

    /// Catch-all error.
    #[error(transparent)]
    Other(#[from] anyhow::Error), // source and Display delegate to anyhow::Error
}

/// Errors thrown by Minion during execution.
///
/// These represent internal Minion C++ exceptions translated into Rust.
///
/// Invalid usage of this library should throw an error before Minion is even run. Therefore, these
/// should be quite rare. Consider creating an issue on
/// [Github](https://github.com/conjure-cp/conjure-oxide) if these occur regularly!
#[derive(Debug, Error, Eq, PartialEq)]
#[non_exhaustive]
pub enum RuntimeError {
    // These closely follow the MinionResult enum in Minion's libwrapper.h.
    /// The model given to Minion is invalid.
    #[error("the given instance is invalid: {0}")]
    InvalidInstance(String),

    /// The solver exceeded its time limit.
    #[error("solver timed out")]
    Timeout,

    /// The solver ran out of memory.
    #[error("solver ran out of memory")]
    MemoryError,

    /// A parse error occurred (e.g. bad variable name, duplicate name).
    #[error("parse error: {0}")]
    ParseError(String),

    /// An invalid argument was provided.
    #[error("invalid argument: {0}")]
    InvalidArgument(String),

    /// An unknown error has occurred.
    #[error("an unknown error has occurred while running minion: {0}")]
    UnknownError(String),
}

/// Check a MinionResult code and convert to Result.
///
/// On error, reads the thread-local error message from minion_error_message().
pub fn check_minion_result(code: u32) -> Result<(), RuntimeError> {
    #[allow(non_upper_case_globals)]
    match code {
        ffi::MinionResult_MINION_OK => Ok(()),
        _ => {
            let msg = unsafe {
                let p = ffi::minion_error_message();
                if p.is_null() {
                    String::new()
                } else {
                    std::ffi::CStr::from_ptr(p).to_string_lossy().into_owned()
                }
            };
            Err(match code {
                ffi::MinionResult_MINION_INVALID_INSTANCE => RuntimeError::InvalidInstance(msg),
                ffi::MinionResult_MINION_TIMEOUT => RuntimeError::Timeout,
                ffi::MinionResult_MINION_MEMORY_ERROR => RuntimeError::MemoryError,
                ffi::MinionResult_MINION_PARSE_ERROR => RuntimeError::ParseError(msg),
                ffi::MinionResult_MINION_INVALID_ARGUMENT => RuntimeError::InvalidArgument(msg),
                _ => RuntimeError::UnknownError(msg),
            })
        }
    }
}
