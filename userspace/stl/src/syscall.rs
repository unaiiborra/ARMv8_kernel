use core::{ffi::c_void, hint::unreachable_unchecked};

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

const fn syscall_code(code: SyscallCode) -> u64 {
    code as u64
}

macro_rules! syscall {
    ($code:expr, $a0:expr, $a1:expr, $a2:expr, $a3:expr, $a4:expr, $a5:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x1") $a1 as u64,
            in("x2") $a2 as u64,
            in("x3") $a3 as u64,
            in("x4") $a4 as u64,
            in("x5") $a5 as u64,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr, $a0:expr, $a1:expr, $a2:expr, $a3:expr, $a4:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x1") $a1 as u64,
            in("x2") $a2 as u64,
            in("x3") $a3 as u64,
            in("x4") $a4 as u64,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr, $a0:expr, $a1:expr, $a2:expr, $a3:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x1") $a1 as u64,
            in("x2") $a2 as u64,
            in("x3") $a3 as u64,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr, $a0:expr, $a1:expr, $a2:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x1") $a1 as u64,
            in("x2") $a2 as u64,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr, $a0:expr, $a1:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x1") $a1 as u64,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr, $a0:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            inout("x0") $a0 as u64 => ret,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
    ($code:expr) => {{
        let ret: i64;
        core::arch::asm!(
            "svc #0",
            lateout("x0") ret,
            in("x8") syscall_code($code),
            options(nostack),
        );
        ret
    }};
}

// ─── Exit ────────────────────────────────────────────────────────────────────

#[inline]
pub fn syscall_exit(code: i32) -> ! {
    unsafe {
        syscall!(SyscallCode::Exit, code);
        unreachable_unchecked();
    };
}

// ─── Print ───────────────────────────────────────────────────────────────────
#[repr(i64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallPrintError {
    InvalidBuf = -1,
}

#[inline]
pub fn syscall_print(buf: *const u8, size: usize) -> Result<(), SyscallPrintError> {
    match unsafe { syscall!(SyscallCode::Print, buf as u64, size as u64) } {
        0 => Ok(()),
        -1 => Err(SyscallPrintError::InvalidBuf),
        r => panic!("syscall_print: unexpected result code {r}"),
    }
}

// ─── Spawn ───────────────────────────────────────────────────────────────────

#[repr(i64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallSpawnError {
    Unmapped = -1,
    Noexec = -2,
}

pub type StartFn = unsafe extern "C" fn(thid: u64, arg: u64);

#[inline]
pub fn syscall_spawn(entry: StartFn, arg: u64) -> Result<u64, SyscallSpawnError> {
    match unsafe { syscall!(SyscallCode::Spawn, entry as usize, arg) } {
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

#[repr(i64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallKillError {
    NotFound = -1,
}


#[inline]
pub fn syscall_kill(thid: u64) -> Result<(), SyscallKillError> {
    match unsafe { syscall!(SyscallCode::Kill, thid) } {
        0 => Ok(()),
        -1 => Err(SyscallKillError::NotFound),
        r => panic!("syscall_kill: unexpected result code {r}"),
    }
}

// ─── Yield ───────────────────────────────────────────────────────────────────

#[inline]
pub fn syscall_yield() {
    unsafe { syscall!(SyscallCode::Yield) };
}

// ─── Mmap ────────────────────────────────────────────────────────────────────

#[repr(i64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallMmapError {
    Err = -1,
    CfgNotSupported = -2,
}

#[derive(Debug, Clone, Copy)]
pub struct MmapProt(u64);

impl MmapProt {
    const F_READ: u64 = 0;
    const F_WRITE: u64 = 1;
    const F_EXEC: u64 = 2;

    pub fn new(read_allowed: bool, write_allowed: bool, exec_allowed: bool) -> Self {
        let mut prot: u64 = 0;
        prot |= (read_allowed as u64) << Self::F_READ;
        prot |= (write_allowed as u64) << Self::F_WRITE;
        prot |= (exec_allowed as u64) << Self::F_EXEC;

        Self(prot)
    }

    pub fn read_allowed(&self) -> bool {
        (self.0 >> Self::F_READ) & 1 != 0
    }

    pub fn write_allowed(&self) -> bool {
        (self.0 >> Self::F_WRITE) & 1 != 0
    }

    pub fn exec_allowed(&self) -> bool {
        (self.0 >> Self::F_EXEC) & 1 != 0
    }
}

#[derive(Debug, Clone, Copy)]
pub struct MmapFlags(u64);

impl MmapFlags {
    const F_ANONYMOUS: u64 = 0;
    const F_FIXED: u64 = 1;

    pub fn new(anonymous: bool, fixed: bool) -> Self {
        let mut flags: u64 = 0;
        flags |= (anonymous as u64) << Self::F_ANONYMOUS;
        flags |= (fixed as u64) << Self::F_FIXED;

        Self(flags)
    }

    pub fn anonymous(&self) -> bool {
        (self.0 >> Self::F_ANONYMOUS) & 1 != 0
    }

    pub fn fixed(&self) -> bool {
        (self.0 >> Self::F_FIXED) & 1 != 0
    }
}

#[inline]
pub fn syscall_mmap(
    addr: *mut c_void,
    length: usize,
    prot: MmapProt,
    flags: MmapFlags,
    fd: u64,
    offset: usize,
) -> Result<*mut c_void, SyscallMmapError> {
    let result = unsafe { syscall!(SyscallCode::Mmap, addr, length, prot.0, flags.0, fd, offset) };

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

#[repr(i64)]
#[derive(Debug, Clone, Copy)]
pub enum SyscallMunmapError {
    Err = -1,
    AlignNotSupported = -2,
}

#[inline]
pub fn syscall_munmap(addr: *mut c_void, length: usize) -> Result<(), SyscallMunmapError> {
    match unsafe { syscall!(SyscallCode::Munmap, addr, length) } {
        0 => Ok(()),
        -1 => Err(SyscallMunmapError::Err),
        -2 => Err(SyscallMunmapError::AlignNotSupported),
        r => panic!("syscall_munmap: unexpected result code {r}"),
    }
}

mod c {

    use core::ffi::c_void;

    // c_api.rs - wrappers planos para C, llaman a syscall directamente
    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_exit(code: i32) -> ! {
        super::syscall_exit(code)
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_print(buf: *const u8, size: usize) -> i64 {
        match super::syscall_print(buf, size) {
            Ok(_) => 0,
            Err(e) => e as i64,
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_spawn(entry: extern "C" fn(u64, u64), arg: u64) -> i64 {
        match super::syscall_spawn(entry, arg) {
            Ok(thid) => thid as i64,
            Err(e) => e as i64,
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_kill(thid: u64) -> i64 {
        if let Err(e) = super::syscall_kill(thid) {
            e as i64
        } else {
            0
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_yield() {
        super::syscall_yield();
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
        let result = super::syscall_mmap(
            addr,
            length,
            super::MmapProt(prot),
            super::MmapFlags(flags),
            fd,
            offset,
        );

        match result {
            Ok(ptr) => ptr as i64,
            Err(e) => e as i64,
        }
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn syscall_munmap(addr: *mut c_void, length: usize) -> i64 {
        if let Err(e) = super::syscall_munmap(addr, length) {
            e as i64
        } else {
            0
        }
    }
}
