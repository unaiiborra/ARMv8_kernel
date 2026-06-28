#![no_std]

use core::ffi::c_void;

use stl::{
    stdio::{print, read},
    stdlib::yield_cpu,
    syscall::{MmapFlags, MmapProt, syscall_kill, syscall_mmap, syscall_munmap, syscall_spawn},
};

extern crate stl;

const N: usize = 20_000;

#[unsafe(no_mangle)]
extern "C" fn main() -> i32 {
    bench_yield(N);
    bench_mmap_munmap(N);
    bench_spawn_kill(N);
    bench_write(N);
    bench_read(N, false);
    0
}

const PAGES: usize = 1;

fn bench_yield(n: usize) {
    for _ in 0..n {
        yield_cpu(); // calls syscall yield
    }
}

fn bench_mmap_munmap(n: usize) {
    for _ in 0..n {
        let ptr = syscall_mmap(
            0 as *mut c_void,
            PAGES * 4096,
            MmapProt::new(true, true, false), /* read && write */
            MmapFlags::new(true, false),      /* anonymous && !fixed */
            0,                                // fd ignored (anonymous)
            0,                                // offset
        )
        .expect("mmap should work with this parameters");

        // commit on demand test
        for i in 0..PAGES {
            unsafe {
                let page_ptr = ptr.add(i * 4096) as *mut u8;
                core::ptr::write_volatile(page_ptr, 0xFF); // write 1 byte
            }
        }

        syscall_munmap(ptr, PAGES * 4096).unwrap();
    }
}

unsafe extern "C" fn nop_thread_entry(_thid: u64, _arg: u64) {
    loop {}
}

fn bench_spawn_kill(n: usize) {
    for _ in 0..n {
        let tid = syscall_spawn(nop_thread_entry, 0).unwrap();
        syscall_kill(tid).unwrap();
    }
}

fn bench_write(n: usize) {
    for _ in 0..n {
        print("Hello from userspace!\n\r");
    }
}

fn bench_read(n: usize, with_data: bool) {
    let mut read_buf: [u8; 1000] = [0; 1000];

    for _ in 0..n {
        loop {
            let slice = read(&mut read_buf).expect("syscall should work");

            if with_data && slice.is_empty() {
                continue;
            }

            break;
        }
    }
}
