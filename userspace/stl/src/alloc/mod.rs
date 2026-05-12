#[allow(hidden_glob_reexports)]
extern crate alloc;

pub use alloc::*;

use core::{alloc::GlobalAlloc, ffi::c_void};

use crate::{printf, stdio::print};

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

        #[cfg(debug_assertions)]
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

        #[cfg(debug_assertions)]
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
        let freed = unsafe { __stl_free(ptr as *mut c_void, layout.size(), layout.align()) };

        #[cfg(debug_assertions)]
        {
            printf!(
                "[dealloc] ptr={:p} size={} align={} freed={}\n\r",
                ptr,
                layout.size(),
                layout.align(),
                freed
            );

            if !freed {
                print("[stl] __stl_free failed!\n\r");
            }
        }
    }
}

// --- TESTING ---

#[repr(C)]
#[derive(Clone, Copy)]
#[cfg(debug_assertions)]
pub struct CacheStats {
    pub arena_count: usize,
    pub used_entries: usize,
    pub slot_size: usize,
}

#[cfg(debug_assertions)]
unsafe extern "C" {
    fn get_allocation_node_count() -> usize;
    fn get_cache_allocation_count() -> usize;
    fn get_mmap_allocation_count() -> usize;
    fn get_cache_stats(out: *mut CacheStats, out_count: *mut usize);
}

#[cfg(debug_assertions)]
pub fn test_allocator() {
    use super::alloc::vec::Vec;

    print("[stl] testing allocator...\n\r");

    // escribe y verifica datos en cada tamaño
    let mut vecs: Vec<Vec<u8>> = Vec::new();
    for size in [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096] {
        let mut v: Vec<u8> = Vec::with_capacity(size);
        for i in 0..size {
            v.push((i & 0xFF) as u8);
        }

        // verificar que lo escrito se lee correctamente
        let mut ok = true;
        for i in 0..size {
            if v[i] != (i & 0xFF) as u8 {
                printf!("[stl] FAIL: data corruption at size={} idx={}\n\r", size, i);
                ok = false;
                break;
            }
        }
        if ok {
            printf!("[stl] OK: data integrity size={}\n\r", size);
        }

        vecs.push(v);
    }

    unsafe {
        let nodes = get_allocation_node_count();
        let cache = get_cache_allocation_count();
        let mmap = get_mmap_allocation_count();

        printf!("[stl] allocation nodes: {}\n\r", nodes);
        printf!("[stl] cache allocs:     {}\n\r", cache);
        printf!("[stl] mmap allocs:      {}\n\r", mmap);

        let mut stats = [CacheStats {
            arena_count: 0,
            used_entries: 0,
            slot_size: 0,
        }; 10];
        let mut count: usize = 0;
        get_cache_stats(stats.as_mut_ptr(), &mut count);

        for i in 0..count {
            let s = &stats[i];
            printf!(
                "[stl] cache slot={:4}  arenas={}  used={}\n\r",
                s.slot_size,
                s.arena_count,
                s.used_entries
            );
        }
    }

    drop(vecs);

    unsafe {
        let nodes = get_allocation_node_count();
        if nodes != 0 {
            printf!("[stl] FAIL: {} nodes still alive after drop\n\r", nodes);
        } else {
            print("[stl] OK: all allocations freed\n\r");
        }
    }

    test_allocator_stress();
}

#[cfg(debug_assertions)]
pub fn test_allocator_stress() {
    use super::alloc::vec::Vec;

    print("[stl] stress testing allocator...\n\r");

    // test 1: muchas allocaciones pequeñas y liberaciones intercaladas
    print("[stl] stress 1: interleaved alloc/free small...\n\r");
    {
        let mut vecs: Vec<Vec<u8>> = Vec::new();
        for round in 0..10 {
            printf!("[stl] round {}\n\r", round);
            for i in 0..100_usize {
                let size = (i % 2048) + 1;
                printf!("[stl] allocating size={}\n\r", size);
                let mut v: Vec<u8> = Vec::with_capacity(size);
                for j in 0..size {
                    v.push((j & 0xFF) as u8);
                }
                vecs.push(v);
            }

            // verifica y libera la mitad
            let drain_count = vecs.len() / 2;
            for i in 0..drain_count {
                let v = &vecs[i];
                for j in 0..v.len() {
                    if v[j] != (j & 0xFF) as u8 {
                        printf!(
                            "[stl] FAIL stress1: corruption round={} i={} j={}\n\r",
                            round,
                            i,
                            j
                        );
                        return;
                    }
                }
            }
            vecs.drain(0..drain_count);
        }
    } // drop el resto

    unsafe {
        let nodes = get_allocation_node_count();
        if nodes != 0 {
            printf!("[stl] FAIL stress1: {} nodes leaked\n\r", nodes);
            return;
        }
    }
    print("[stl] OK stress 1\n\r");

    // test 2: allocaciones grandes (mmap)
    print("[stl] stress 2: large mmap allocs...\n\r");
    {
        let sizes = [4096, 8192, 16384, 32768, 65536, 131072];
        let mut vecs: Vec<Vec<u8>> = Vec::new();

        for &size in &sizes {
            let mut v: Vec<u8> = Vec::with_capacity(size);
            for i in 0..size {
                v.push((i & 0xFF) as u8);
            }

            for i in 0..size {
                if v[i] != (i & 0xFF) as u8 {
                    printf!("[stl] FAIL stress2: corruption size={} i={}\n\r", size, i);
                    return;
                }
            }
            printf!("[stl] OK: mmap size={}\n\r", size);
            vecs.push(v);
        }
    }

    unsafe {
        let nodes = get_allocation_node_count();
        if nodes != 0 {
            printf!("[stl] FAIL stress2: {} nodes leaked\n\r", nodes);
            return;
        }
    }
    print("[stl] OK stress 2\n\r");

    // test 3: fragmentation — aloca, libera alternos, vuelve a alocar
    print("[stl] stress 3: fragmentation...\n\r");
    {
        let mut vecs: Vec<Option<Vec<u8>>> = Vec::new();

        for i in 0..200_usize {
            let size = (i % 512) + 4;
            let mut v: Vec<u8> = Vec::with_capacity(size);
            for j in 0..size {
                v.push((j & 0xFF) as u8);
            }
            vecs.push(Some(v));
        }

        // libera los pares
        for i in (0..vecs.len()).step_by(2) {
            vecs[i] = None;
        }

        // realloca en los huecos
        for i in (0..vecs.len()).step_by(2) {
            let size = (i % 512) + 4;
            let mut v: Vec<u8> = Vec::with_capacity(size);
            for j in 0..size {
                v.push((j & 0xFF) as u8);
            }
            vecs[i] = Some(v);
        }

        // verifica todo
        for i in 0..vecs.len() {
            if let Some(v) = &vecs[i] {
                for j in 0..v.len() {
                    if v[j] != (j & 0xFF) as u8 {
                        printf!("[stl] FAIL stress3: corruption i={} j={}\n\r", i, j);
                        return;
                    }
                }
            }
        }
    }

    unsafe {
        let nodes = get_allocation_node_count();
        if nodes != 0 {
            printf!("[stl] FAIL stress3: {} nodes leaked\n\r", nodes);
            return;
        }
    }
    print("[stl] OK stress 3\n\r");

    print("[stl] all stress tests passed!\n\r");
}
