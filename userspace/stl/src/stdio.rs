use core::fmt::{self, Write};

use crate::alloc::*;
use crate::vfs::{FileDescriptor, VfsError};

pub const STDIN_FD: FileDescriptor = FileDescriptor::new(0);
pub const STDOUT_FD: FileDescriptor = FileDescriptor::new(1);
pub const STDERR_FD: FileDescriptor = FileDescriptor::new(2);

// Read
pub fn read(buf: &mut [u8]) -> Result<&mut [u8], VfsError> {
    let count = STDIN_FD.read(buf)?;

    Ok(&mut buf[0..count])
}

// Writes
#[inline(always)]
fn out_write_str(fd: &FileDescriptor, s: &str) -> fmt::Result {
    let written_count = fd.write(s.as_bytes()).map_err(|_| fmt::Error)?;

    if written_count != s.as_bytes().len() {
        Err(fmt::Error)
    } else {
        Ok(())
    }
}

struct Stdout;
impl Write for Stdout {
    #[inline(always)]
    fn write_str(&mut self, s: &str) -> fmt::Result {
        out_write_str(&STDOUT_FD, s)
    }
}

struct Stderr;
impl Write for Stderr {
    #[inline(always)]
    fn write_str(&mut self, s: &str) -> fmt::Result {
        out_write_str(&STDERR_FD, s)
    }
}

pub fn print(msg: &str) {
    Stdout.write_str(msg).expect("Stdout error");
}

pub fn print_err(msg: &str) {
    Stderr.write_str(msg).expect("Stderr error");
}

pub fn printf(args: fmt::Arguments) {
    Stdout.write_fmt(args).expect("Formatting error")
}

pub fn printf_err(args: fmt::Arguments) {
    Stderr.write_fmt(args).expect("Formatting error")
}

#[macro_export]
macro_rules! printf {
    ($($arg:tt)*) => {
        $crate::stdio::printf(format_args!($($arg)*))
    };
}

#[macro_export]
macro_rules! printf_err {
    ($($arg:tt)*) => {
        $crate::stdio::printf_err(format_args!($($arg)*))
    };
}

mod c {
    use core::ffi::c_char;

    #[unsafe(no_mangle)]
    pub extern "C" fn print(msg: *const c_char) {
        if msg.is_null() {
            return;
        }

        let mut i: usize = 0;
        while unsafe { *msg.add(i) } != b'\0' {
            i += 1;
        }

        if i == 0 {
            return;
        }

        super::print(unsafe {
            core::str::from_utf8_unchecked_mut(core::slice::from_raw_parts_mut(msg as *mut u8, i))
        });
    }
}
