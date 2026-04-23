#![no_std]
#![allow(non_snake_case)]
#![allow(non_camel_case_types)]
#![allow(special_module_name)]



mod lib;

use core::panic::PanicInfo;

// -- Panic handler ---



#[unsafe(no_mangle)]
extern "C" fn rust_test_panic() -> ! {
    panic!("Test working :)");
}

#[panic_handler]
fn rust_panic(_info: &PanicInfo) -> ! {
    loop {}
}
