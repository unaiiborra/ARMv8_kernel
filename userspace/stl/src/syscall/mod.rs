use core::ffi::c_void;

use crate::stdio::print;

unsafe extern "C" {
    pub fn syscall(a0: u64, a1: u64, a2: u64, a3: u64, a4: u64, a5: u64, code: SyscallCode) -> i64;
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallCode {
    Exit,
    Print,
    Spawn,
    Kill,
    Yield,
    Mmap,
    Munmap,
}

// ─── Exit ────────────────────────────────────────────────────────────────────

pub fn syscall_exit(code: i32) -> ! {
    c::syscall_exit(code);
}

// ─── Print ───────────────────────────────────────────────────────────────────
#[derive(Debug, Clone, Copy)]
pub enum SyscallPrintError {
    InvalidBuf,
}

pub fn syscall_print(buf: *const u8, size: usize) -> Result<(), SyscallPrintError> {
    match c::syscall_print(buf, size) {
        0 => Ok(()),
        -1 => Err(SyscallPrintError::InvalidBuf),
        r => panic!("syscall_print: unexpected result code {r}"),
    }
}

// ─── Spawn ───────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy)]
pub enum SyscallSpawnError {
    Unmapped,
    Noexec,
}

pub type SpawnFn = extern "C" fn(thid: u64, arg: u64);

pub fn syscall_spawn(entry: SpawnFn, arg: u64) -> Result<u64, SyscallSpawnError> {
    match c::syscall_spawn(entry, arg) {
        -1 => Err(SyscallSpawnError::Unmapped),
        -2 => Err(SyscallSpawnError::Noexec),
        r => {
            if r < 0 {
                panic!("syscall_spawn: unexpected result code {r}")
            } else {
                return Ok(r as u64);
            }
        }
    }
}

// ─── Kill ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy)]
pub enum SyscallKillError {
    NotFound,
}

pub fn syscall_kill(thid: u64) -> Result<(), SyscallKillError> {
    match c::syscall_kill(thid) {
        0 => Ok(()),
        -1 => Err(SyscallKillError::NotFound),
        r => panic!("syscall_kill: unexpected result code {r}"),
    }
}

// ─── Yield ───────────────────────────────────────────────────────────────────

pub fn syscall_yield() {
    c::syscall_yield();
}

// ─── Mmap ────────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy)]
pub enum SyscallMmapError {
    Err,
    CfgNotSupported,
}

pub struct MmapProt {
    prot: u64,
}

impl MmapProt {
    const F_READ: u64 = 0;
    const F_WRITE: u64 = 1;
    const F_EXEC: u64 = 2;

    pub fn new(read_allowed: bool, write_allowed: bool, exec_allowed: bool) -> Self {
        let mut prot: u64 = 0;
        prot |= (read_allowed as u64) << Self::F_READ;
        prot |= (write_allowed as u64) << Self::F_WRITE;
        prot |= (exec_allowed as u64) << Self::F_EXEC;

        Self { prot }
    }

    pub fn read_allowed(&self) -> bool {
        (self.prot >> Self::F_READ) & 1 != 0
    }

    pub fn write_allowed(&self) -> bool {
        (self.prot >> Self::F_WRITE) & 1 != 0
    }

    pub fn exec_allowed(&self) -> bool {
        (self.prot >> Self::F_EXEC) & 1 != 0
    }
}

#[derive(Debug, Clone, Copy)]
pub struct MmapFlags {
    flags: u64,
}

impl MmapFlags {
    const F_ANONYMOUS: u64 = 0;
    const F_FIXED: u64 = 1;

    pub fn new(anonymous: bool, fixed: bool) -> Self {
        let mut flags: u64 = 0;
        flags |= (anonymous as u64) << Self::F_ANONYMOUS;
        flags |= (fixed as u64) << Self::F_FIXED;

        Self { flags }
    }

    pub fn anonymous(&self) -> bool {
        (self.flags >> Self::F_ANONYMOUS) & 1 != 0
    }

    pub fn fixed(&self) -> bool {
        (self.flags >> Self::F_FIXED) & 1 != 0
    }
}

pub fn syscall_mmap(
    addr: *mut c_void,
    length: usize,
    prot: MmapProt,
    flags: MmapFlags,
    fd: u64,
    offset: usize,
) -> Result<*mut c_void, SyscallMmapError> {
    let result = c::syscall_mmap(addr, length, prot.prot, flags.flags, fd, offset);

    if result >= 0 {
        return Ok(result as *mut c_void);
    }

    match result {
        -1 => Err(SyscallMmapError::Err),
        -2 => Err(SyscallMmapError::CfgNotSupported),
        r => panic!("syscall_mmap: unexpected result code {r}"),
    }
}

// ─── Munmap ──────────────────────────────────────────────────────────────────

#[derive(Debug, Clone, Copy)]
pub enum SyscallMunmapError {
    Err,
    AlignNotSupported,
}

pub fn syscall_munmap(addr: *mut c_void, length: usize) -> Result<(), SyscallMunmapError> {
    match c::syscall_munmap(addr, length) {
        0 => Ok(()),
        -1 => Err(SyscallMunmapError::Err),
        -2 => Err(SyscallMunmapError::AlignNotSupported),
        r => panic!("syscall_munmap: unexpected result code {r}"),
    }
}

mod c {
    use crate::syscall::syscall;
    use core::ffi::c_void;

    // c_api.rs - wrappers planos para C, llaman a syscall directamente
    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_exit(code: i32) -> ! {
        unsafe { syscall(code as u64, 0, 0, 0, 0, 0, super::SyscallCode::Exit) };
        unreachable!();
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_print(buf: *const u8, size: usize) -> i64 {
        unsafe {
            syscall(
                buf as u64,
                size as u64,
                0,
                0,
                0,
                0,
                super::SyscallCode::Print,
            )
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_spawn(entry: extern "C" fn(u64, u64), arg: u64) -> i64 {
        unsafe { syscall(entry as u64, arg, 0, 0, 0, 0, super::SyscallCode::Spawn) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_kill(thid: u64) -> i64 {
        unsafe { syscall(thid, 0, 0, 0, 0, 0, super::SyscallCode::Kill) }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_yield() {
        unsafe { syscall(0, 0, 0, 0, 0, 0, super::SyscallCode::Yield) };
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_mmap(
        addr: *mut c_void,
        length: usize,
        prot: u64,
        flags: u64,
        fd: u64,
        offset: usize,
    ) -> i64 {
        let result = unsafe {
            syscall(
                addr as u64,
                length as u64,
                prot,
                flags,
                fd,
                offset as u64,
                super::SyscallCode::Mmap,
            )
        };

        #[cfg(debug_assertions)]
        if result >= 0 {
            let page = result as *const u8;
            for i in 0..length {
                let byte = unsafe { *page.add(i) };
                if byte != 0 {
                    panic!(
                        "syscall_mmap: page not zeroed at offset {} value=0x{:02x}",
                        i, byte
                    );
                }
            }
        }

        result
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_munmap(addr: *mut c_void, length: usize) -> i64 {
        unsafe {
            syscall(
                addr as u64,
                length as u64,
                0,
                0,
                0,
                0,
                super::SyscallCode::Munmap,
            )
        }
    }
}
