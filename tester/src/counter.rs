use atomic_counter::*;

lazy_static! {
    static ref COUNTER: RelaxedCounter = RelaxedCounter::new(1);
}

pub fn get_unique_value() -> usize {
    COUNTER.inc()
}

pub fn get_unique_name(base: &str, ext: &str) -> String {
    format!("{}{}{}", base, get_unique_value(), ext)
}
