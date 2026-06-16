#[allow(hidden_glob_reexports)]
extern crate alloc;
pub use alloc::*;
use core::{alloc::GlobalAlloc, ffi::c_void};

#[cfg(feature = "allocator_logs")]
use crate::{printf, stdio::print};

#[global_allocator]
static STL_ALLOCATOR: StlAllocator = StlAllocator {};

unsafe extern "C" {
    fn __stl_malloc(layout_size: usize, layout_align: usize, zeroed: bool) -> *mut c_void;
    fn __stl_free(addr: *mut c_void, layout_size: usize, layout_align: usize) -> bool;
}

struct StlAllocator;
unsafe impl GlobalAlloc for StlAllocator {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = unsafe { __stl_malloc(layout.size(), layout.align(), false) };

        #[cfg(feature = "allocator_logs")]
        {
            printf!(
                "[alloc] ptr={:p} size={} align={}\n\r",
                addr,
                layout.size(),
                layout.align()
            );

            if addr.is_null() {
                panic!("[stl] malloc failed!\n\r");
            }
        }

        addr as *mut u8
    }

    unsafe fn alloc_zeroed(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = unsafe { __stl_malloc(layout.size(), layout.align(), true) };

        #[cfg(feature = "allocator_logs")]
        {
            printf!(
                "[alloc_zeroed] ptr={:p} size={} align={}\n\r",
                addr,
                layout.size(),
                layout.align()
            );

            if addr.is_null() {
                panic!("[stl] __stl_malloc failed!");
            }
        }

        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        let _freed = unsafe { __stl_free(ptr as *mut c_void, layout.size(), layout.align()) };

        #[cfg(feature = "allocator_logs")]
        {
            printf!(
                "[dealloc] ptr={:p} size={} align={} freed={}\n\r",
                ptr,
                layout.size(),
                layout.align(),
                _freed
            );

            if !_freed {
                print("[stl] __stl_free failed!\n\r");
            }
        }
    }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn alloc(bytes: usize) -> *mut c_void {
    unsafe { __stl_malloc(bytes, bytes.next_power_of_two(), false) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn zalloc(bytes: usize) -> *mut c_void {
    unsafe { __stl_malloc(bytes, bytes.next_power_of_two(), false) }
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn free(ptr: *mut c_void) {
    if !unsafe { __stl_free(ptr, 0, 0) } {
        panic!("invalid ptr provided, probably due to a double free");
    };
}
