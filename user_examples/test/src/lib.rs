#![no_std]

use stl::printf;

extern crate stl;

#[unsafe(no_mangle)]
pub extern "C" fn rsmain() -> i32 {
    printf!("Hello from Rust!");

    0
}
