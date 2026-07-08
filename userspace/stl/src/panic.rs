use core::panic::PanicInfo;
use core::sync::atomic::{AtomicBool, Ordering};

use crate::stdio::STDERR_FD;
use crate::stdio::buffer_writer::StaticBufferWriter;
use crate::stdlib::exit;
use core::fmt::Write;

static PANIC_IN_PROGRESS: AtomicBool = AtomicBool::new(false);
static mut PANIC_MESSAGE_BYTES: [u8; 4096] = [0u8; 4096];

#[panic_handler]
fn stl_panic(info: &PanicInfo) -> ! {
    if PANIC_IN_PROGRESS.swap(true, Ordering::SeqCst) {
        exit(-1);
    }
    let array_ref = unsafe { &mut *&raw mut PANIC_MESSAGE_BYTES };
    let mut buffer = StaticBufferWriter::new(&mut array_ref[..]);

    let _ = write!(buffer, "\n\r=== PANIC ===\n\r");

    if let Some(location) = info.location() {
        let _ = write!(
            buffer,
            "[location] {}:{}:{}\n\r",
            location.file(),
            location.line(),
            location.column()
        );
    } else {
        let _ = write!(buffer, "[location] unknown\n\r");
    }

    if let Some(msg) = info.message().as_str() {
        let _ = write!(buffer, "[message]  {}\n\r", msg);
    }

    let _ = write!(buffer, "=============\n\r");
    let _ = buffer.write_fd(STDERR_FD);

    exit(-1)
}
