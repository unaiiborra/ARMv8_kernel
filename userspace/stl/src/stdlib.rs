use crate::syscall::{syscall_exit, syscall_yield};

pub fn exit(code: i32) -> ! {
    syscall_exit(code)
}

pub fn yield_cpu() {
    syscall_yield();
}

mod c {
    #[unsafe(no_mangle)]
    pub extern "C" fn exit(code: i32) -> ! {
        super::exit(code)
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn r#yield() {
        super::yield_cpu();
    }
}
