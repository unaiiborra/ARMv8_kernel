use core::{alloc::GlobalAlloc, ffi::c_void};

use crate::stdio::print;

#[global_allocator]
static STL_ALLOCATOR: StlAlloc = StlAlloc {};

unsafe extern "C" {
    fn __stl_malloc(layout_size: usize, layout_align: usize, zeroed: bool) -> *mut c_void;
    fn __stl_free(addr: *mut c_void, layout_size: usize, layout_align: usize) -> bool;
}

struct StlAlloc;
unsafe impl GlobalAlloc for StlAlloc {
    unsafe fn alloc(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = unsafe { __stl_malloc(layout.size(), layout.align(), false) };

        if cfg!(debug_assertions) && addr.is_null() {
            panic!("[stl] malloc failed!");
        }

        addr as *mut u8
    }

    unsafe fn alloc_zeroed(&self, layout: core::alloc::Layout) -> *mut u8 {
        let addr = unsafe { __stl_malloc(layout.size(), layout.align(), true) };

        if cfg!(debug_assertions) && addr.is_null() {
            panic!("[stl] __stl_malloc failed!");
        }

        addr as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: core::alloc::Layout) {
        let freed = unsafe { __stl_free(ptr as *mut c_void, layout.size(), layout.align()) };

        if cfg!(debug_assertions) && !freed {
            print("[stl] __stl_free failed!");
        }
    }
}
