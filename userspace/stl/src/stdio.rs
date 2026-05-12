use core::fmt::{self, Write};

use crate::syscall::syscall_print;

struct Stdout;

impl Write for Stdout {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        syscall_print(s.as_ptr(), s.len()).or_else(|_| Err(fmt::Error))
    }
}

pub fn print(msg: &str) {
    Stdout.write_str(msg).expect("SYSCALL PRINT invalid buf");
}

pub fn printf(args: fmt::Arguments) {
    Stdout.write_fmt(args).expect("formatting error")
}

#[macro_export]
macro_rules! printf {
    ($($arg:tt)*) => {
        $crate::stdio::printf(format_args!($($arg)*))
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
