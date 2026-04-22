/// A light scoped wrapper over a raw *mut pointer.
///
/// Implements destruction of the pointer when it goes out of scope, but provides no other
/// guarantees.
#[non_exhaustive]
pub struct Scoped<T> {
    pub ptr: *mut T,
    destructor: fn(*mut T),
}

// Could use
// https://doc.rust-lang.org/std/alloc/trait.Allocator.html with box (in nightly only)
// or
// https://docs.rs/scopeguard/latest/scopeguard/
// instead
impl<T> Scoped<T> {
    pub unsafe fn new(ptr: *mut T, destructor: fn(*mut T)) -> Scoped<T> {
        Scoped { ptr, destructor }
    }
}

// https://doc.rust-lang.org/nomicon/destructors.html
impl<T> Drop for Scoped<T> {
    fn drop(&mut self) {
        (self.destructor)(self.ptr);
    }
}
