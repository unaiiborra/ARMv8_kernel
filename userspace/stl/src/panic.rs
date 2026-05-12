use core::panic::PanicInfo;

use crate::printf;
use crate::{stdio::print, stdlib::exit};

use core::fmt::{self, Write};

struct PanicWriter {
    buf: [u8; 4096],
    pos: usize,
}

impl PanicWriter {
    const fn new() -> Self {
        Self {
            buf: [0u8; 4096],
            pos: 0,
        }
    }
}

impl Write for PanicWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let bytes = s.as_bytes();
        let remaining = self.buf.len() - self.pos;
        let to_write = bytes.len().min(remaining);
        self.buf[self.pos..self.pos + to_write].copy_from_slice(&bytes[..to_write]);
        self.pos += to_write;
        Ok(())
    }
}

#[panic_handler]
fn stl_panic(info: &PanicInfo) -> ! {
    print("\n\r=== PANIC ===\n\r");

    if let Some(location) = info.location() {
        printf!(
            "[location] {}:{}:{}\n\r",
            location.file(),
            location.line(),
            location.column()
        );
    } else {
        print("[location] unknown\n\r");
    }

    if let Some(msg) = info.message().as_str() {
        printf!("[message]  {}\n\r", msg);
    } else {
        let mut writer = PanicWriter::new();
        let _ = write!(writer, "{}", info.message());
        let msg = core::str::from_utf8(&writer.buf[..writer.pos]).unwrap_or("<utf8 error>");
        printf!("[message]  {}\n\r", msg);
    }

    print("=============\n\r");

    exit(-1)
}
