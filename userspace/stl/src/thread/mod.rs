use core::ffi::c_void;

use crate::{
    alloc::{alloc, free},
    syscall::{StartFn, SyscallKillError, SyscallSpawnError, syscall_kill, syscall_spawn},
};

pub type ThreadCreateError = SyscallSpawnError;
pub type ThreadKillError = SyscallKillError;

pub type Thid = u64;

unsafe extern "C" {
    fn __stl_thread_start(thid: u64, arg: u64);
}

#[repr(C)]
#[derive(Clone, Copy)]
struct ThreadArgs {
    arg: u64,
    start_fn: *const c_void,
}
impl ThreadArgs {
    pub fn unpack(&self) -> (u64, StartFn) {
        (self.arg, unsafe { core::mem::transmute(self.start_fn) })
    }
}

#[inline]
pub fn thread_create(entry: StartFn, arg: u64) -> Result<Thid, ThreadCreateError> {
    let arg = unsafe {
        let arguments = alloc(size_of::<ThreadArgs>()) as *mut ThreadArgs;

        *arguments = ThreadArgs {
            arg,
            start_fn: entry as usize as *const c_void,
        };

        arguments as usize as u64
    };

    syscall_spawn(__stl_thread_start, arg).map_err(|e| {
        unsafe { free(arg as *mut c_void) };
        e
    })
}

#[inline]
pub fn thread_kill(thid: Thid) -> Result<(), ThreadKillError> {
    syscall_kill(thid)
}

#[unsafe(no_mangle)]
unsafe extern "C" fn __stl_setup_thread_entry(thid: u64, arguments_ptr: *mut ThreadArgs) -> ! {
    unsafe {
        let (arg, start_fn) = (*arguments_ptr).unpack();

        free(arguments_ptr as *mut c_void);

        start_fn(thid, arg);
    };

    syscall_kill(thid).map_err(|e| {
        panic!(
            "Could not kill thread {} due to KILL error {}",
            thid, e as i64
        );
    });

    unreachable!("thread should have already been killed by itself");
}

mod c {
    #[unsafe(no_mangle)]
    pub extern "C" fn thread_create(entry: crate::syscall::StartFn, arg: u64) -> i64 {
        match super::thread_create(entry, arg) {
            Ok(thid) => thid as i64,
            Err(e) => e as i64,
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn thread_kill(thid: u64) -> i64 {
        if let Err(e) = super::thread_kill(thid) {
            e as i64
        } else {
            0
        }
    }
}
