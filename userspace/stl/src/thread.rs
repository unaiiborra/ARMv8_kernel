use crate::syscall::{SpawnFn, SyscallKillError, SyscallSpawnError, syscall_kill, syscall_spawn};

pub type ThreadCreateError = SyscallSpawnError;
pub type ThreadKillError = SyscallKillError;

pub type Thid = u64;

#[inline]
pub fn thread_create(entry: SpawnFn, arg: u64) -> Result<Thid, ThreadCreateError> {
    syscall_spawn(entry, arg)
}

#[inline]
pub fn thread_kill(thid: Thid) -> Result<(), ThreadKillError> {
    syscall_kill(thid)
}

mod c {
    unsafe extern "C" {
        fn syscall_spawn(entry: extern "C" fn(u64, u64), arg: u64) -> i64;
        fn syscall_kill(thid: u64) -> i64;
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn thread_create(entry: crate::syscall::SpawnFn, arg: u64) -> i64 {
        unsafe {
            syscall_spawn(entry, arg)
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn thread_kill(thid: u64) -> i64 {
        unsafe {
          syscall_kill(thid)
        }
    }
}
