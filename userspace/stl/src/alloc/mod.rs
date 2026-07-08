#[allow(hidden_glob_reexports)]
extern crate alloc;
pub use alloc::*;
use core::{alloc::GlobalAlloc, ffi::c_void};

use crate::lock::Spinlock;

#[cfg(feature = "allocator_logs")]
mod allocator_logs_imports {
    pub use crate::stdio::STDOUT_FD;
    pub use crate::stdio::buffer_writer::StaticBufferWriter;
    pub use core::fmt::Write;
}

#[global_allocator]
pub static STL_ALLOCATOR: StlAllocator = StlAllocator {};
static STL_ALLOCATOR_LOCK: Spinlock<i8> = Spinlock::new(0);

unsafe extern "C" {
    fn __stl_malloc(layout_size: usize, layout_align: usize, zeroed: bool) -> *mut c_void;
    fn __stl_free(addr: *mut c_void, layout_size: usize, layout_align: usize) -> bool;
}

pub struct StlAllocator;
unsafe impl GlobalAlloc for StlAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = STL_ALLOCATOR_LOCK
            .locked(|_| unsafe { __stl_malloc(layout.size(), layout.align(), false) });

        #[cfg(feature = "allocator_logs")]
        {
            use allocator_logs_imports::*;

            let mut bytes = [0u8; 1024];
            let mut writer = StaticBufferWriter::new(&mut bytes);

            write!(
                writer,
                "[alloc] ptr={:p} size={} align={}\n\r",
                addr,
                layout.size(),
                layout.align()
            )
            .unwrap();

            writer.write_fd(STDOUT_FD).unwrap();
        }

        assert!(!addr.is_null(), "[stl] __stl_malloc failed!");

        addr as *mut u8
    }

    unsafe fn alloc_zeroed(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = STL_ALLOCATOR_LOCK
            .locked(|_| unsafe { __stl_malloc(layout.size(), layout.align(), true) });

        #[cfg(feature = "allocator_logs")]
        {
            use allocator_logs_imports::*;

            let mut bytes = [0u8; 1024];
            let mut writer = StaticBufferWriter::new(&mut bytes);

            write!(
                writer,
                "[alloc_zeroed] ptr={:p} size={} align={}\n\r",
                addr,
                layout.size(),
                layout.align()
            )
            .unwrap();

            writer.write_fd(STDOUT_FD).unwrap();
        }

        assert!(!addr.is_null(), "[stl] __stl_malloc failed!");

        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        let freed = STL_ALLOCATOR_LOCK
            .locked(|_| unsafe { __stl_free(ptr as *mut c_void, layout.size(), layout.align()) });

        #[cfg(feature = "allocator_logs")]
        {
            use allocator_logs_imports::*;

            let mut bytes = [0u8; 1024];
            let mut writer = StaticBufferWriter::new(&mut bytes);

            write!(
                writer,
                "[dealloc] ptr={:p} size={} align={} freed={}\n\r",
                ptr,
                layout.size(),
                layout.align(),
                freed
            )
            .unwrap();

            if !freed {
                write!(writer, "[stl] __stl_free failed!\n\r").unwrap();
            }

            writer.write_fd(STDOUT_FD).unwrap();
        }

        assert!(freed, "global allocator free failed!");
    }
}

#[unsafe(no_mangle)]
unsafe extern "C" fn alloc(bytes: usize) -> *mut c_void {
    STL_ALLOCATOR_LOCK.locked(|_| unsafe { __stl_malloc(bytes, bytes.next_power_of_two(), false) })
}

#[unsafe(no_mangle)]
unsafe extern "C" fn zalloc(bytes: usize) -> *mut c_void {
    STL_ALLOCATOR_LOCK.locked(|_| unsafe { __stl_malloc(bytes, bytes.next_power_of_two(), true) })
}

#[unsafe(no_mangle)]
unsafe extern "C" fn free(ptr: *mut c_void) {
    if !STL_ALLOCATOR_LOCK.locked(|_| unsafe { __stl_free(ptr, 0, 0) }) {
        panic!("invalid ptr provided, probably due to a double free");
    };
}
