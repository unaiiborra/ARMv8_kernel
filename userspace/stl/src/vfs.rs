use num_enum::TryFromPrimitive;

use crate::syscall::{syscall_read, syscall_write};

#[repr(i32)]
#[derive(Debug, Clone, Copy, TryFromPrimitive)]
pub enum VfsError {
    BadF = -1,            // Invalid fd
    NoDev = -2,           // Device not registered
    OpNotSupported = -3,  // Op not supported
    InvalidArgument = -4, // Invalid argument
}

#[derive(Clone, Copy)]
pub struct FileDescriptor(u32);
impl FileDescriptor {
    pub const fn new(fd: u32) -> Self {
        Self(fd)
    }

    /// unsafe because the fd should ve validated when used by using the methods of this struct
    pub unsafe fn get_value(&self) -> u32 {
        self.0
    }

    pub fn read(&self, buf: &mut [u8]) -> Result<usize, VfsError> {
        if buf.is_empty() {
            return Err(VfsError::InvalidArgument);
        }

        syscall_read(*self, buf.as_mut_ptr(), buf.len())
    }

    pub fn write(&self, buf: &[u8]) -> Result<usize, VfsError> {
        if buf.is_empty() {
            return Err(VfsError::InvalidArgument);
        }

        syscall_write(*self, buf.as_ptr(), buf.len())
    }
}
