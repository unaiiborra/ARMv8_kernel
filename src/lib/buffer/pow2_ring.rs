use core::{ffi::c_void, ptr};

#[repr(C)]
pub struct pow2_ring_buffer {
    buf: *mut c_void,
    // Size of elements, not bytes
    size: usize,
    // In bytes
    t_size: usize,
    tail: usize,
    head: usize,
    overwrite: bool,
}
impl pow2_ring_buffer {
    pub fn new<T>(buf: *mut c_void, size: usize, overwrite: bool) -> pow2_ring_buffer {
        if (size & (size - 1)) != 0 || (buf as usize) % size_of::<T>() != 0 {
            panic!("pow2_ring_buffer: Size must be pow of 2 and be aligned to the t_size");
        }

        return Self {
            buf,
            size,
            t_size: size_of::<T>(),
            tail: 0,
            head: 0,
            overwrite,
        };
    }

    pub const fn new_uninit<T>() -> pow2_ring_buffer {
        pow2_ring_buffer {
            buf: core::ptr::null_mut(),
            size: 0,
            t_size: 0,
            tail: 0,
            head: 0,
            overwrite: false,
        }
    }

    #[unsafe(no_mangle)]
    extern "C" fn pow2_ring_buffer_new(
        buf: *mut c_void,
        size: usize,
        t_size: usize,
        overwrite: bool,
    ) -> pow2_ring_buffer {
        if (size & (size - 1)) != 0 || (buf as usize) % t_size != 0 {
            panic!("pow2_ring_buffer: Size must be pow of 2 and be aligned to the t_size");
        }

        return Self {
            buf,
            size,
            t_size,
            tail: 0,
            head: 0,
            overwrite,
        };
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.head == self.tail
    }

    #[inline]
    pub fn is_full(&self) -> bool {
        ((self.head + 1) & self.mask()) == self.tail
    }

    #[inline]
    pub fn mask(&self) -> usize {
        self.size - 1
    }

    pub fn push(&mut self, v: *const c_void) -> bool {
        let is_full = self.is_full();

        if !is_full {
            unsafe {
                let dst = (self.buf as *mut u8).add(self.head * self.t_size);
                ptr::copy_nonoverlapping(v as *const u8, dst, self.t_size);
            }
            self.head = (self.head + 1) & self.mask();
        } else if self.overwrite {
            unsafe {
                let dst = (self.buf as *mut u8).add(self.head * self.t_size);
                ptr::copy_nonoverlapping(v as *const u8, dst, self.t_size);
            }

            self.head = (self.head + 1) & self.mask();
            self.tail = (self.tail + 1) & self.mask();
        }

        return !is_full;
    }

    #[unsafe(no_mangle)]
    extern "C" fn pow2_ring_buffer_push(rb: *mut pow2_ring_buffer, v: *const c_void) -> bool {
        if let Some(buf) = unsafe { rb.as_mut() } {
            return buf.push(v);
        }

        panic!("Invalid struct provided!");
    }

    pub fn pop(&mut self, out: *mut c_void) -> bool {
        if self.is_empty() {
            return false;
        }

        unsafe {
            let src = (self.buf as *const u8).add(self.tail * self.t_size);
            ptr::copy_nonoverlapping(src, out as *mut u8, self.t_size);
        }

        self.tail = (self.tail + 1) & self.mask();

        true
    }

    #[unsafe(no_mangle)]
    extern "C" fn pow2_ring_buffer_pop(rb: *mut pow2_ring_buffer, out: *mut c_void) -> bool {
        if let Some(buf) = unsafe { rb.as_mut() } {
            return buf.pop(out);
        }

        panic!("Invalid struct provided!");
    }
}

// Rust implementation
pub struct RsPow2RingBuffer<T: Copy, const SIZE: usize, const OVERWRITE: bool> {
    buf: [T; SIZE],
    tail: usize,
    head: usize,
}
impl<T: Copy, const SIZE: usize, const OVERWRITE: bool> RsPow2RingBuffer<T, SIZE, OVERWRITE> {
    const MASK: usize = SIZE - 1;

    pub const fn new(buf: [T; SIZE]) -> Self {
        debug_assert!(SIZE.is_power_of_two(), "SIZE must be power of two");
        debug_assert!(SIZE != 0, "SIZE must not be zero");

        Self {
            buf,
            tail: 0,
            head: 0,
        }
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.head == self.tail
    }

    #[inline]
    pub fn is_full(&self) -> bool {
        ((self.head + 1) & Self::MASK) == self.tail
    }

    pub fn push(&mut self, v: T) -> bool {
        let next = (self.head + 1) & Self::MASK;

        if next == self.tail {
            if !OVERWRITE {
                return false;
            }
            self.tail = (self.tail + 1) & Self::MASK;
        }

        self.buf[self.head] = v;
        self.head = next;

        true
    }

    pub fn pop(&mut self) -> Option<T> {
        if self.is_empty() {
            return None;
        }

        let v = self.buf[self.tail];

        self.tail = (self.tail + 1) & Self::MASK;

        return Some(v);
    }
}
