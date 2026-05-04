//! Compact digest of a solution multiset.
//!
//! NOTE: WE ARE HASHING SOLUTIONS, NOT STORING THEM. The tester used to
//! materialise every solution into a `Vec<Vec<i64>>` so it could sort and
//! compare two runs for equality. On hard random instances at the
//! `--max-solutions` cap (default 10 million), that worked out to ~1.2 GB
//! per run × two runs in flight × N concurrent test threads — which is
//! what produced the OOM we saw in the JOBS=5 stress sweep, NOT minion's
//! pre-allocated address space (calloc'd large blocks are demand-paged on
//! both Linux and macOS).
//!
//! This module replaces the storage with an order-independent multiset
//! hash: `sum (mod 2^64)` of `SipHash-13` of each solution. Two runs
//! agree iff their digests agree (and their counts match). Hash
//! collisions are theoretically possible — a 64-bit space gives roughly
//! 2^-64 probability per comparison, and across ~260 K comparisons in a
//! full sweep the cumulative collision probability is on the order of
//! 1.4e-14. The trade is intentional: we accept that vanishing risk in
//! exchange for O(1) memory per run regardless of solution count.
//!
//! Wrapping addition is used (not XOR) because XOR cancels duplicate
//! entries — adding the same solution twice would leave the digest
//! unchanged, masking duplication bugs. Wrapping addition is still
//! commutative + associative (so order is irrelevant) but keeps
//! multiplicities visible in the digest.
//!
//! A small bounded sample of solutions is retained alongside the digest
//! so failure messages can show concrete examples on mismatch — without
//! the sample we'd only know "the digests differ", which is poor for
//! debugging. The cap is small (16 solutions) so memory stays O(1).

use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};

/// Maximum number of solutions retained verbatim for failure
/// diagnostics. Anything beyond this is hashed and dropped.
pub const SAMPLE_CAP: usize = 16;

/// Multiset digest of solutions seen by a single Minion run.
///
/// Add solutions via [`SolutionDigest::add`]; compare via [`PartialEq`]
/// (which checks both `sum` and `count`).
#[derive(Debug, Clone, Default)]
pub struct SolutionDigest {
    /// Wrapping sum of per-solution 64-bit hashes. Order-independent
    /// and multiset-correct (duplicates change the sum).
    pub sum: u64,
    /// Number of solutions hashed in. A useful cross-check alongside
    /// `sum` — if two runs agree on `sum` but disagree on `count`,
    /// something is structurally wrong (and the digest comparison
    /// would still flag it via the count difference).
    pub count: u64,
    /// First [`SAMPLE_CAP`] solutions retained verbatim, for failure
    /// diagnostics only. NOT used in equality comparison.
    pub sample: Vec<Vec<i64>>,
}

impl SolutionDigest {
    pub fn new() -> Self {
        Self::default()
    }

    /// Hash one solution into the digest. Stores it in `sample` if the
    /// cap has not yet been reached.
    pub fn add(&mut self, sol: Vec<i64>) {
        let mut h = DefaultHasher::new();
        sol.hash(&mut h);
        self.sum = self.sum.wrapping_add(h.finish());
        self.count += 1;
        if self.sample.len() < SAMPLE_CAP {
            self.sample.push(sol);
        }
    }
}

impl PartialEq for SolutionDigest {
    fn eq(&self, other: &Self) -> bool {
        self.sum == other.sum && self.count == other.count
    }
}
impl Eq for SolutionDigest {}
