#![no_std]

use core::sync::atomic::AtomicU64;

use stl::{
    lock::Spinlock,
    printf,
    stdlib::yield_cpu,
    thread::{thread_create, thread_kill},
};

extern crate stl;

const THREADS: usize = 200;
static LOCK: Spinlock<i32> = Spinlock::new(0);
static FINISHED: AtomicU64 = AtomicU64::new(0);

#[unsafe(no_mangle)]
unsafe extern "C" fn secondary_entry(thid: u64, arg: u64) {
    LOCK.locked(|_| printf!("Thread {} spawned!", arg));

    FINISHED.fetch_add(1, core::sync::atomic::Ordering::Release);

    thread_kill(thid).expect("could not kill self thread");
}

#[unsafe(no_mangle)]
extern "C" fn main() -> i32 {
    for i in 1..=THREADS {
        thread_create(secondary_entry, i as u64).unwrap();
    }

    while FINISHED.load(core::sync::atomic::Ordering::Relaxed) != THREADS as u64 {
        yield_cpu();
    }

    return 0;
}
