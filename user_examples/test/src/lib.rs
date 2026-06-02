#![no_std]

use stl::printf;

extern crate stl;

#[unsafe(no_mangle)]
pub extern "C" fn print_thid(thid: u64) {
    printf!("Hello from thread {thid}\n\r");
}
