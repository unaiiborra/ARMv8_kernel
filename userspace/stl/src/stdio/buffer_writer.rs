use crate::{
    alloc::vec::Vec,
    vfs::{FileDescriptor, VfsError},
};
use core::{cmp::min, fmt::Write};

pub struct BufferWriter(Vec<u8>);
impl BufferWriter {
    pub fn new() -> BufferWriter {
        BufferWriter(Vec::with_capacity(128))
    }

    pub fn clear(&mut self) {
        self.0.clear();
    }

    pub fn flush(&mut self, fd: FileDescriptor) -> Result<usize, VfsError> {
        let result = self.write_fd(fd);
        self.clear();

        result
    }

    pub fn write_fd(&mut self, fd: FileDescriptor) -> Result<usize, VfsError> {
        fd.write(&self.0)
    }
}
impl Write for BufferWriter {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        self.0.extend_from_slice(s.as_bytes());
        Ok(())
    }
}

pub struct StaticBufferWriter<'a> {
    buf: &'a mut [u8],
    len: usize,
}
impl<'a> StaticBufferWriter<'a> {
    pub fn new(buf: &'a mut [u8]) -> StaticBufferWriter<'a> {
        StaticBufferWriter { buf, len: 0 }
    }

    pub fn capacity(&self) -> usize {
        self.buf.len()
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn is_empty(&self) -> bool {
        self.len == 0
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.buf[..self.len]
    }

    pub fn clear(&mut self) {
        self.len = 0;
    }

    pub fn flush(&mut self, fd: FileDescriptor) -> Result<usize, VfsError> {
        let result = self.write_fd(fd);
        self.clear();

        result
    }

    pub fn write_fd(&self, fd: FileDescriptor) -> Result<usize, VfsError> {
        fd.write(self.as_bytes())
    }
}

impl<'a> Write for StaticBufferWriter<'a> {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        let bytes = s.as_bytes();
        let start = self.len;

        if start >= self.buf.len() {
            return Ok(());
        }

        let available = self.buf.len() - start;
        let write_len = min(bytes.len(), available);
        let end = start + write_len;

        self.buf[start..end].copy_from_slice(&bytes[..write_len]);
        self.len = end;

        Ok(())
    }
}
