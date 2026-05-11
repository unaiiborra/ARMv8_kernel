use crate::syscall::syscall_print;

pub fn print(msg: &str) {
    syscall_print(msg.as_ptr(), msg.len()).expect("invalid buffer provided");
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
