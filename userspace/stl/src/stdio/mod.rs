pub mod buffer_writer;

use crate::alloc::*;
use crate::stdio::buffer_writer::BufferWriter;
use crate::vfs::{FileDescriptor, VfsError};
use core::fmt::{self, Write};

pub const STDIN_FD: FileDescriptor = FileDescriptor::new(0);
pub const STDOUT_FD: FileDescriptor = FileDescriptor::new(1);
pub const STDERR_FD: FileDescriptor = FileDescriptor::new(2);

// Read
pub fn read(buf: &mut [u8]) -> Result<&mut [u8], VfsError> {
    let count = STDIN_FD.read(buf)?;

    Ok(&mut buf[0..count])
}

// Write
pub fn print(msg: &str) {
    STDOUT_FD.write(msg.as_bytes()).expect("Stdout error");
}

pub fn print_err(msg: &str) {
    STDERR_FD.write(msg.as_bytes()).expect("Stderr error");
}

pub fn printf(args: fmt::Arguments) {
    let mut buffer = BufferWriter::new();
    buffer.write_fmt(args).expect("Formatting error");
    buffer.write_fd(STDOUT_FD).expect("Stdout error");
}

pub fn printf_err(args: fmt::Arguments) {
    let mut buffer = BufferWriter::new();
    buffer.write_fmt(args).expect("Formatting error");
    buffer.write_fd(STDERR_FD).expect("Stderr error");
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
