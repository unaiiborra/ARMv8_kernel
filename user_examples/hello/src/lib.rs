#![no_std]

extern crate stl;

#[unsafe(no_mangle)]
extern "C" fn main() -> i32 /* The STL automatically calls exit with the provided code */ {
    for i in 1..=5 {
        stl::printf!("Hello from userspace! {}", i); // rust manages the formatting and the STL handles the write syscalls
    }
    
    /* Other common STL provided functions
    thread_create(entry, arg)
    thread_kill(thid)
    syscall_mmap(addr, length, prot, flags, fd, offset)
    syscall_munmap(addr, length) */

    0
}
