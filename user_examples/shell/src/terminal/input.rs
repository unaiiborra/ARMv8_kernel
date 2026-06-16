use stl::{
    alloc::{boxed::Box, collections::vec_deque::VecDeque},
    printf_err,
    stdio::STDIN_FD,
    stdlib::{exit, yield_cpu},
};

pub struct TermInput {
    read_buffer: Box<[u8; Self::READ_BUFFER_SIZE]>,
    buffer: VecDeque<u8>,
    error_count: usize,
}
impl TermInput {
    const READ_BUFFER_SIZE: usize = 4096 * 4;
    const MAX_BUFFER_SIZE: usize = 4096 * 16;
    const MAX_ERROR_COUNT: usize = 5;

    fn read_input(&mut self) {
        if self.buffer.len() >= Self::MAX_BUFFER_SIZE {
            return;
        }

        match STDIN_FD.read(&mut *self.read_buffer) {
            Ok(count) => {
                if count > 0 {
                    self.buffer.extend(&self.read_buffer[0..count]);

                    if count == Self::READ_BUFFER_SIZE {
                        self.read_input();
                    }
                }
            }
            Err(_) => {
                if self.error_count >= Self::MAX_ERROR_COUNT {
                    printf_err!("[SHELL] terminating due to multiple errors");
                    exit(1);
                }

                self.error_count += 1;
            }
        }
    }

    pub fn receive(&mut self) -> Option<u8> {
        self.read_input();
        self.buffer.pop_front().or_else(|| {
            yield_cpu();
            None
        })
    }
}
impl Default for TermInput {
    fn default() -> Self {
        Self {
            read_buffer: Box::new([0; Self::READ_BUFFER_SIZE]),
            buffer: VecDeque::new(),
            error_count: 0,
        }
    }
}
