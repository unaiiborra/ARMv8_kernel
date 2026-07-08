#![no_std]

extern crate stl;

use core::fmt::Write;
use core::sync::atomic::AtomicU64;
use stl::{
    stdio::{STDOUT_FD, buffer_writer::StaticBufferWriter, print},
    stdlib::yield_cpu,
    thread::{thread_create, thread_kill},
};

const THREAD_COUNT: usize = 200;
static FINISHED: AtomicU64 = AtomicU64::new(0);

#[unsafe(no_mangle)]
unsafe extern "C" fn secondary_entry(thid: u64, arg: u64) {
    let mut arr = [0u8; 512];
    let mut buf = StaticBufferWriter::new(&mut arr);

    write!(buf, "Hello from process B thread {} ({})!\n\r", thid, arg).expect("Formatting failed");

    buf.flush(STDOUT_FD).expect("STDOUT failed!");

    FINISHED.fetch_add(1, core::sync::atomic::Ordering::Release);
    thread_kill(thid).expect("could not kill self thread");
}

#[unsafe(no_mangle)]
extern "C" fn main() -> i32 {
    for i in 1..=THREAD_COUNT {
        thread_create(secondary_entry, i as u64).unwrap();
    }

    while FINISHED.load(core::sync::atomic::Ordering::Relaxed) != THREAD_COUNT as u64 {
        yield_cpu();
    }

    print("Process B finished OK!\n\r");

    return 0;
}
