#![no_std]

use stl::stdio::print;

extern crate stl;

#[unsafe(no_mangle)]
pub extern "C" fn rsmain() {
    print("Hello world from rust!");
}
