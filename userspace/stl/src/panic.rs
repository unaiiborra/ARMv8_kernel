use core::panic::PanicInfo;

use crate::{stdio::print, stdlib::exit};

#[panic_handler]
fn stl_panic(_info: &PanicInfo) -> ! {
    let location = _info.location().unwrap_or_else(|| exit(-1));

    print("\n\rPanic!\n\r[file] ");
    print(location.file());

    print(_info.message().as_str().unwrap_or_else(|| exit(-1)));

    exit(-1)
}
