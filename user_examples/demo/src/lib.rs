#![no_std]

use core::ffi::c_int;

use stl::{alloc::test_allocator, printf};

extern crate stl;

#[unsafe(no_mangle)]
extern "C" fn main() -> c_int {
    #[cfg(debug_assertions)]
    {
        test_allocator();
    }

    printf!("Test ok!\n\r");

    return 0;
}
